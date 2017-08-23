/*
 * AliasHintCollector.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in AliasHintCollector.h (see this file to learn further about the 
 * goals of such class).
 */

#include "AliasHintCollector.h"
#include "IPIDUnit.h"
#include "UDPUnreachablePortUnit.h"
#include "TimestampCheckUnit.h"
#include "ReverseDNSUnit.h"
#include "../../common/thread/Thread.h"

AliasHintCollector::AliasHintCollector(TreeNETEnvironment *env)
{
    this->env = env;
    tokenCounter = 1;
    
    printSteps = false;
    debug = false;
    
    unsigned short displayMode = env->getDisplayMode();
    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        printSteps = true;
    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_DEBUG)
        debug = true;
}

AliasHintCollector::~AliasHintCollector()
{
}

void AliasHintCollector::collect()
{
    ostream *out = env->getOutputStream();
    IPLookUpTable *table = env->getIPTable();
    
    // Sorts and removes duplicata (an ingress interface of a neighborhood can be a contra-pivot).
    this->IPsToProbe.sort(InetAddress::smaller);
    InetAddress previous(0);
    for(list<InetAddress>::iterator i = this->IPsToProbe.begin(); i != this->IPsToProbe.end(); ++i)
    {
        InetAddress current = (*i);
        
        if(current == previous)
        {
            this->IPsToProbe.erase(i--);
        }
        
        previous = current;
    }
    
    // Amounts of threads and collected IP-IDs
    unsigned short maxThreads = env->getMaxThreads();
    unsigned short nbIPIDs = env->getNbIPIDs();
    unsigned long int nbIPs = (unsigned long int) this->IPsToProbe.size();
    if(nbIPs == 0)
    {
        return;
    }
    unsigned short nbThreads = 1;
    
    // Computes the amount of required threads (valid for each step)
    if(nbIPs > (unsigned long int) maxThreads)
        nbThreads = maxThreads;
    else
        nbThreads = (unsigned short) nbIPs;
    
    // Initializes the array of threads
    Thread **th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;

    // Does a copy of the IPs list for next hints
    list<InetAddress> backUp1(this->IPsToProbe);
    list<InetAddress> backUp2(this->IPsToProbe);
    list<InetAddress> backUp3(this->IPsToProbe);
    
    if(printSteps)
    {
        (*out) << "1. IP-ID collection... " << std::flush;
        if(debug)
        {
            (*out) << endl;
        }
    }
    
    // Creates the IP-ID tuple arrays
    for(list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        IPIDTuple *newArray = new IPIDTuple[nbIPIDs];
        IPIDTuples.insert(pair<InetAddress, IPIDTuple*>((*it), newArray));
    }

    /*
     * Starts scheduling for IP-ID collection; the code proceeds by rounds, i.e., it will probe 
     * all candidate during the first round and wait for a reply. It is only after getting a reply 
     * or a timeout for each that the next round can start.
     */
    
    list<InetAddress> roundTargets = this->IPsToProbe;
    for(unsigned short i = 0; i < nbIPIDs; i++)
    {
        list<IPIDTuple> tuples;
        
        // Schedules one IPIDUnit per IP
        unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / maxThreads;
        unsigned short j = 0;
        while(roundTargets.size() > 0)
        {
            InetAddress IPToProbe(roundTargets.front());
            roundTargets.pop_front();
            
            // If current thread not null, waits for completion before scheduling the new IPIDUnit
            if(th[j] != NULL)
            {
                th[j]->join();
                IPIDUnit *curUnit = (IPIDUnit*) th[j]->getRunnable();
                
                if(env->debugMode())
                    (*out) << curUnit->getDebugLog();
                
                if(curUnit->hasExploitableResults())
                    tuples.push_back(curUnit->getTuple());
                
                delete th[j];
                th[j] = NULL;
            }
            
            // Schedules the new IPIDUnit
            Runnable *task = NULL;
            try
            {
                task = new IPIDUnit(this->env, 
                                    this, 
                                    IPToProbe, 
                                    DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range), 
                                    DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range) + range - 1, 
                                    DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                    DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
                th[j] = new Thread(task);
            }
            catch(SocketException &se)
            {
                // Cleans remaining threads (if any is set)
                for(unsigned short k = 0; k < nbThreads; k++)
                {
                    if(th[k] != NULL)
                    {
                        th[k]->join();
                        delete th[k];
                        th[k] = NULL;
                    }
                }
                
                delete[] th;
                
                throw StopException();
            }
            catch(ThreadException &te)
            {
                (*out) << "\nUnable to create more threads." << endl;
            
                delete task;
            
                // Cleans remaining threads (if any is set)
                for(unsigned short k = 0; k < nbThreads; k++)
                {
                    if(th[k] != NULL)
                    {
                        th[k]->join();
                        delete th[k];
                        th[k] = NULL;
                    }
                }
                
                delete[] th;
            
                throw StopException();
            }
            
            // Starts and moves to next thread
            th[j]->start();
            
            j++;
            if(j == nbThreads)
                j = 0;
            
            // 0,01s of delay before next thread
            Thread::invokeSleep(TimeVal(0, 10000));
            
            if(env->isStopping())
                break;
        }

        // Waiting for all remaining threads to complete
        for(j = 0; j < nbThreads; j++)
        {
            if(th[j] != NULL)
            {
                th[j]->join();
                IPIDUnit *curUnit = (IPIDUnit*) th[j]->getRunnable();
                
                if(env->debugMode())
                    (*out) << curUnit->getDebugLog();
                
                if(curUnit->hasExploitableResults())
                    tuples.push_back(curUnit->getTuple());
                
                delete th[j];
                th[j] = NULL;
            }
        }
        
        /*
         * End of current round, we do a quick post-processing of the tuples to schedule the next 
         * round and re-order IP-IDs which clearly form a sequence but came out-of-order due to 
         * network delays.
         */
        
        tuples.sort(IPIDTuple::compareByTime);
        for(list<IPIDTuple>::iterator it = tuples.begin(); it != tuples.end(); ++it)
            roundTargets.push_back((*it).target);
        
        tuples.sort(IPIDTuple::compareByID);
        IPIDTuple prev;
        unsigned short reordered = 0;
        for(list<IPIDTuple>::iterator it = tuples.begin(); it != tuples.end(); ++it)
        {
            if(prev.probeToken != 0)
            {
                IPIDTuple cur = (*it);
                if(cur.echo || prev.echo)
                    continue;
                
                unsigned short diff = cur.IPID - prev.IPID;
                if(diff < IPID_MAX_DIFF && cur.probeToken < prev.probeToken)
                {
                    reordered++;
                    // Moves the problematic token backwards
                    IPIDTuple *prevBis = &(*it);
                    for(list<IPIDTuple>::iterator it2 = it; it2 != tuples.begin(); --it2)
                    {
                        if(it2 == it)
                        {
                            it2--;
                            continue;
                        }
                        IPIDTuple *curBis = &(*it2);
                        if(curBis->echo)
                            break;
                        
                        unsigned short diffBis = prevBis->IPID - curBis->IPID;
                        if(diffBis < IPID_MAX_DIFF && curBis->probeToken > prevBis->probeToken)
                        {
                            unsigned long tempToken = curBis->probeToken;
                            curBis->probeToken = prevBis->probeToken;
                            prevBis->probeToken = tempToken;
                            prevBis = &(*it2);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                prev = (*it);
            }
        }
        
        // Stores the tuples in the IPIDTuples map
        while(tuples.size() > 0)
        {
            IPIDTuple curTuple = tuples.front();
            tuples.pop_front();
            
            map<InetAddress, IPIDTuple*>::iterator res = IPIDTuples.find(curTuple.target);
            if(res != IPIDTuples.end())
            {
                IPIDTuple *tuplesArray = res->second;
                tuplesArray[i] = curTuple;
            }
        }
        
        if(env->isStopping())
            break;
        
        if(printSteps)
        {
            if(i == 0)
                (*out) << endl;
            (*out) << "Done with round " << (i + 1) << ".";
            if(reordered > 0)
            {
                if(reordered > 1)
                    (*out) << " Changed position of " << reordered << " tokens.";
                else
                    (*out) << " Changed position of one token.";
            }
            (*out) << endl;
        }
    }
    
    if(env->isStopping())
    {
        delete[] th;
        throw StopException();
    }
    
    /*
     * For each initial target IP, computes the final data (uses backUp1 for this) to store in 
     * the IP dictionnary, using the map.
     */
    
    for(list<InetAddress>::iterator it = backUp1.begin(); it != backUp1.end(); ++it)
    {
        InetAddress curIP = (*it);
        map<InetAddress, IPIDTuple*>::iterator res = IPIDTuples.find(curIP);
        if(res != IPIDTuples.end())
        {
            IPIDTuple *tuplesArray = res->second;
            IPTableEntry *targetEntry = table->lookUp(curIP);
            if(targetEntry == NULL) // Should not occur, ideally, but just in case (avoids crash)
                continue;
            
            unsigned char inferredInitialTTL = 0;
            bool differentTTLs = false;
            for(unsigned short i = 0; i < nbIPIDs; i++)
            {
                IPIDTuple curTuple = tuplesArray[i];
                
                targetEntry->setProbeToken(i, curTuple.probeToken);
                targetEntry->setIPIdentifier(i, curTuple.IPID);
                if(curTuple.echo)
                    targetEntry->setEcho(i);
                
                // Only if the same initial TTL was observed on every previous tuple
                if(!differentTTLs)
                {
                    // Infers initial TTL value of the reply packet for the current tuple
                    unsigned char initialTTL = 0;
                    unsigned short replyTTLAsShort = (unsigned short) curTuple.replyTTL;
                    if(replyTTLAsShort > 128)
                        initialTTL = (unsigned char) 255;
                    else if(replyTTLAsShort > 64)
                        initialTTL = (unsigned char) 128;
                    else if(replyTTLAsShort > 32)
                        initialTTL = (unsigned char) 64;
                    else
                        initialTTL = (unsigned char) 32;
                    
                    // Checks if distinct initial TTLs are observed (it should always be the same)
                    if(inferredInitialTTL == 0)
                    {
                        inferredInitialTTL = initialTTL;
                    }
                    else if(inferredInitialTTL != initialTTL)
                    {
                        differentTTLs = true;
                    }
                }
                
                // Computes delay with next entry
                if(i < nbIPIDs - 1)
                {
                    IPIDTuple nextTuple = tuplesArray[i + 1];
                    
                    unsigned long time1, time2;
                    time1 = curTuple.timeValue.tv_sec * 1000000 + curTuple.timeValue.tv_usec;
                    time2 = nextTuple.timeValue.tv_sec * 1000000 + nextTuple.timeValue.tv_usec;
                            
                    unsigned long delay = time2 - time1;
                    targetEntry->setDelay(i, delay);
                }
            }
            
            if(inferredInitialTTL > 0 && !differentTTLs)
                targetEntry->setEchoInitialTTL(inferredInitialTTL);
            
            delete[] tuplesArray;
        }
    }
    IPIDTuples.clear(); // Empties map for next call to collect() (collisions on 0.0.0.0 can occur)
    
    if(printSteps)
    {
        // Different output for debug mode
        if(debug)
        {
            (*out) << "\nDone with IP-ID collection for this neighborhood.\n" << endl;
        }
        else
            (*out) << "Done." << endl;
    }
    
    // Now schedules threads to try the UDP/ICMP Port Unreachable approach
    if(printSteps)
    {
        (*out) << "2. Probing each IP with UDP (unreachable port)... " << std::flush;
        if(debug)
        {
            (*out) << endl;
        }
    }
    
    unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / maxThreads;
    unsigned short i = 0;
    while(backUp1.size() > 0)
    {
        InetAddress IPToProbe = backUp1.front();
        backUp1.pop_front();
        
        if(th[i] != NULL)
        {
            th[i]->join();
            UDPUnreachablePortUnit *udpThread = (UDPUnreachablePortUnit*) th[i]->getRunnable();
            
            // Dump probe logs on the console output
            if(debug)
            {
                (*out) << udpThread->getDebugLog();
            }
            
            delete th[i];
            th[i] = NULL;
        }
        
        Runnable *task = NULL;
        try
        {
            task = new UDPUnreachablePortUnit(env, 
                                              IPToProbe, 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                              DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                              DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
        
            th[i] = new Thread(task);
        }
        catch(SocketException &se)
        {
            (*out) << endl;
        
            // Cleans remaining threads (if any is set)
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                if(th[j] != NULL)
                {
                    th[j]->join();
                    delete th[j];
                    th[j] = NULL;
                }
            }
            
            delete[] th;
            
            throw StopException();
        }
        catch(ThreadException &te)
        {
            (*out) << "\nUnable to create more threads." << endl;
        
            delete task;
            
            // Cleans remaining threads (if any is set)
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                if(th[j] != NULL)
                {
                    th[j]->join();
                    delete th[j];
                    th[j] = NULL;
                }
            }
            
            delete[] th;
            
            throw StopException();
        }
        
        th[i]->start();
        
        i++;
        if(i == maxThreads)
            i = 0;
        
        // 0,1s of delay before next thread (if same router, avoids to "bomb" it)
        Thread::invokeSleep(TimeVal(0, 100000));
        
        if(env->isStopping())
            break;
    }
    
    // Waits for all remaining threads to complete
    for(i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            UDPUnreachablePortUnit *udpThread = (UDPUnreachablePortUnit*) th[i]->getRunnable();
            
            // Dump probe logs on the console output
            if(debug)
            {
                (*out) << udpThread->getDebugLog();
            }
            
            delete th[i];
            th[i] = NULL;
        }
    }
    
    if(env->isStopping())
    {
        delete[] th;
        throw StopException();
    }
    
    if(printSteps)
    {
        // Different output for debug mode
        if(debug)
        {
            (*out) << "\nDone with UDP probing for this neighborhood.\n" << endl;
        }
        else
            (*out) << "Done." << endl;
    }
    
    // Now schedules to check if each IP is compatible with prespecified timestamp option...
    if(printSteps)
    {
        (*out) << "3. Sending ICMP timestamp request to each IP... " << std::flush;
        if(debug)
        {
            (*out) << endl;
        }
    }
    
    i = 0;
    while(backUp2.size() > 0)
    {
        InetAddress IPToProbe = backUp2.front();
        backUp2.pop_front();
        
        if(th[i] != NULL)
        {
            th[i]->join();
            TimestampCheckUnit *tsThread = (TimestampCheckUnit*) th[i]->getRunnable();
            
            // Dump probe logs on the console output
            if(debug)
            {
                (*out) << tsThread->getDebugLog();
            }
            
            delete th[i];
            th[i] = NULL;
        }
        
        Runnable *task = NULL;
        try
        {
            task = new TimestampCheckUnit(env, 
                                          IPToProbe, 
                                          DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                          DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                          DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                          DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
            th[i] = new Thread(task);
        }
        catch(SocketException &se)
        {
            (*out) << endl;
        
            // Cleans remaining threads (if any is set)
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                if(th[j] != NULL)
                {
                    th[j]->join();
                    delete th[j];
                    th[j] = NULL;
                }
            }
        
            delete[] th;
            
            throw StopException();
        }
        catch(ThreadException &te)
        {
            (*out) << "\nUnable to create more threads." << endl;
        
            delete task;
        
            // Cleans remaining threads (if any is set)
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                if(th[j] != NULL)
                {
                    th[j]->join();
                    delete th[j];
                    th[j] = NULL;
                }
            }
        
            delete[] th;
            
            throw StopException();
        }
        
        th[i]->start();
        
        i++;
        if(i == maxThreads)
            i = 0;
        
        // 0,1s of delay before next thread (if same router, avoids to "bomb" it)
        Thread::invokeSleep(TimeVal(0, 100000));
        
        if(env->isStopping())
            break;
    }
    
    // Waits for all remaining threads to complete
    for(i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            TimestampCheckUnit *tsThread = (TimestampCheckUnit*) th[i]->getRunnable();
            
            // Dump probe logs on the console output
            if(debug)
            {
                (*out) << tsThread->getDebugLog();
            }
            
            delete th[i];
            th[i] = NULL;
        }
    }
    
    if(env->isStopping())
    {
        delete[] th;
        throw StopException();
    }
    
    if(printSteps)
    {
        // Different output for debug mode
        if(debug)
        {
            (*out) << "\nDone with timestamp requests for this neighborhood.\n" << endl;
        }
        else
            (*out) << "Done." << endl;
    }
    
    /*
     * N.B.: reverse DNS relying on the gethostbyname() function in C (<netdb.h>), there is no 
     * particular debug messages here since the usual prober classes are not involved in this.
     */
    
    // Now schedules to resolve host names of each IP by reverse DNS
    if(printSteps)
        (*out) << "4. Reverse DNS... " << std::flush;
    
    i = 0;
    while(backUp3.size() > 0)
    {
        InetAddress IPToProbe = backUp3.front();
        backUp3.pop_front();
        
        if(th[i] != NULL)
        {
            th[i]->join();
            delete th[i];
            th[i] = NULL;
        }
        
        try
        {
            th[i] = new Thread(new ReverseDNSUnit(env, IPToProbe));
        }
        catch(ThreadException &te)
        {
            (*out) << "\nUnable to create more threads." << endl;
        
            // Cleans remaining threads (if any is set)
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                if(th[j] != NULL)
                {
                    th[j]->join();
                    delete th[j];
                    th[j] = NULL;
                }
            }
            
            delete[] th;
        
            throw StopException();
        }
        
        th[i]->start();
        
        i++;
        if(i == maxThreads)
            i = 0;
        
        // 0,01s of delay before next thread
        Thread::invokeSleep(TimeVal(0, 10000));
        
        if(env->isStopping())
            break;
    }
    
    // Waits for all remaining threads to complete
    for(i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            delete th[i];
            th[i] = NULL;
        }
    }
    
    delete[] th;
    
    if(env->isStopping())
    {
        throw StopException();
    }
    
    /*
     * No condition on printSteps here because the "Done" is actually bringing closure to the 
     * "Collecting hints..." message printed by calling code and is therefore still relevant.
     */
    
    (*out) << "Done." << endl;
    if(debug)
        (*out) << endl; // Additionnal line break in debug mode
}

unsigned long int AliasHintCollector::getProbeToken()
{
    unsigned long int token = tokenCounter;
    tokenCounter++;
    return token;
}
