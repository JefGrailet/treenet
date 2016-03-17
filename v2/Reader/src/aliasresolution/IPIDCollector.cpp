/*
 * IPIDCollector.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: grailet
 *
 * Implements the class defined in IPIDCollector.h (see this file to learn further about the goals 
 * of such class).
 */

#include "IPIDCollector.h"
#include "IPIDUnit.h"
#include "../common/thread/Thread.h"

IPIDCollector::IPIDCollector(TreeNETEnvironment *e, 
                             AliasHintCollector *p, 
                             InetAddress t, 
                             unsigned short offs):
env(e), 
parent(p), 
target(t), 
offset(offs)
{
}

IPIDCollector::~IPIDCollector()
{
}

void IPIDCollector::run()
{
    // Avoids probing null IPs (routing artifacts)
    if(this->target == InetAddress("0.0.0.0"))
        return;

    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *targetEntry = table->lookUp(this->target); // Necessarily exists (see AliasHintCollector.cpp)
    unsigned short nbThreads = env->getNbIPIDs();
    unsigned short maxThreads = env->getMaxThreads(); // For range (see below)
    
    // Just in case (normally, should not occur, as stated above)
    if(targetEntry == NULL)
        return;
    
    /*
     * N.B.: since AliasHintCollector takes care of scheduling with respect to the maximum amount 
     * of threads allowed by the user, this class does neither check nbThreads nor performs a 
     * circular visit of the thread array.
     */
    
    // Initializes an array of threads
    Thread **th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;

    // Starts spawning threads
    unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / maxThreads;
    for(unsigned long int i = 0; i < nbThreads; i++)
    {
        unsigned short trueOffset = this->offset + i;
        th[i] = new Thread(new IPIDUnit(this->env, 
                                        this->parent, 
                                        this->target, 
                                        DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (trueOffset * range), 
                                        DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (trueOffset * range) + range - 1, 
                                        DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                        DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
        
        th[i]->start();
        
        // 0,01s of delay before next thread
        Thread::invokeSleep(TimeVal(0, 10000));
    }

    // Waiting for all threads to complete and getting token, IP-ID, time value tuples
    list<IPIDTuple*> tuples;
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            IPIDUnit *curUnit = (IPIDUnit*) th[i]->getRunnable();
            if(curUnit->hasExploitableResults())
            {
                tuples.push_back(curUnit->getTuple());
            }
            delete th[i];
            th[i] = NULL;
        }
    }
    
    // Sorts and stores collected data in targetEntry
    tuples.sort(IPIDTuple::compare);
    unsigned short index = 0;
    unsigned char inferredInitialTTL = 0;
    bool differentTTLs = false;
    while(tuples.size() > 0)
    {
        IPIDTuple *cur = tuples.front();
        tuples.pop_front();
        
        targetEntry->setProbeToken(index, cur->probeToken);
        targetEntry->setIPIdentifier(index, cur->IPID);
        if(cur->echo)
            targetEntry->setEcho(index);
        
        // Only if the same initial TTL was observed on every previous tuple
        if(!differentTTLs)
        {
            // Infers initial TTL value of the reply packet for the current tuple
            unsigned char initialTTL = 0;
            unsigned short replyTTLAsShort = (unsigned short) cur->replyTTL;
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
        
        // Computes delay with next entry (no pop_front() this time)
        if(tuples.size() > 0)
        {
            IPIDTuple *next = tuples.front();
            
            unsigned long time1, time2;
            time1 = cur->timeValue.tv_sec * 1000000 + cur->timeValue.tv_usec;
            time2 = next->timeValue.tv_sec * 1000000 + next->timeValue.tv_usec;
                    
            unsigned long delay = time2 - time1;
            targetEntry->setDelay(index, delay);
        }
        
        // Frees memory used by current tuple and moves on
        delete cur;
        index++;
    }
    
    if(inferredInitialTTL > 0 && !differentTTLs)
        targetEntry->setEchoInitialTTL(inferredInitialTTL);
    
    delete[] th;
    
    // We're done with this IP.
}
