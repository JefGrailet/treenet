/*
 * AliasHintCollector.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * Implements the class defined in AliasHintCollector.h (see this file to learn further about the 
 * goals of such class).
 */

#include "AliasHintCollector.h"
#include "IPIDCollector.h"
#include "ReverseDNSUnit.h"
#include "../../common/thread/Thread.h"

AliasHintCollector::AliasHintCollector(TreeNETEnvironment *env)
{
    this->env = env;
    tokenCounter = 1;
}

AliasHintCollector::~AliasHintCollector()
{
}

void AliasHintCollector::collect()
{
    IPLookUpTable *table = env->getIPTable();
    unsigned short maxThreads = env->getMaxThreads();
    unsigned short nbIPIDs = env->getNbIPIDs();
    unsigned long int nbIPs = (unsigned long int) IPsToProbe.size();
    if(nbIPs == 0)
    {
        return;
    }
    unsigned short nbThreads = 1;
    
    // Computes the amount of required threads (+1 is due to collector thread itself)
    unsigned short maxCollectors = maxThreads / (nbIPIDs + 1);
    if(nbIPs > (unsigned long int) maxCollectors)
        nbThreads = maxCollectors;
    else
        nbThreads = (unsigned short) nbIPs;
    
    // Initializes an array of threads
    Thread **th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;

    // Does a copy of the IPs list for host name retrieval (after IP-ID retrieval)
    list<InetAddress> backUpIPsToProbe(IPsToProbe);

    // Starts scheduling for IP-ID retrieval
    unsigned short j = 0;
    for(unsigned long int i = 0; i < nbIPs; i++)
    {
        InetAddress IPToProbe(IPsToProbe.front());
        IPsToProbe.pop_front();
        
        if(th[j] != NULL)
        {
            th[j]->join();
            delete th[j];
            th[j] = NULL;
        }
        
        // Creates entry in IP table if it does not exist
        if(table->lookUp(IPToProbe) == NULL)
        {
            IPTableEntry *newEntry = table->create(IPToProbe);
            newEntry->setTTL(currentTTL);
        }
        
        th[j] = new Thread(new IPIDCollector(env, this, IPToProbe, j * nbIPIDs));
        
        th[j]->start();
        
        j++;
        if(j == nbThreads)
            j = 0;
        
        // 0,01s of delay before next thread
        Thread::invokeSleep(TimeVal(0, 10000));
    }

    // Waiting for all remaining threads to complete
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            delete th[i];
            th[i] = NULL;
        }
    }
    
    delete[] th;
    
    // Re-sizes th[] for reverse DNS (because there is a single thread per IP)
    if(nbIPs > (unsigned long int) maxThreads)
        nbThreads = maxThreads;
    else
        nbThreads = (unsigned short) nbIPs;
    
    th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;
    
    // Now schedules to resolve host names of each IP by reverse DNS
    j = 0;
    for(unsigned long int i = 0; i < nbIPs; i++)
    {
        InetAddress IPToProbe = backUpIPsToProbe.front();
        backUpIPsToProbe.pop_front();
        
        if(th[j] != NULL)
        {
            th[j]->join();
            delete th[j];
            th[j] = NULL;
        }
        
        th[j] = new Thread(new ReverseDNSUnit(env, IPToProbe));
        th[j]->start();
        
        j++;
        if(j == maxThreads)
            j = 0;
        
        // 0,01s of delay before next thread
        Thread::invokeSleep(TimeVal(0, 10000));
    }
    
    // Waiting for all remaining threads to complete
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        if(th[i] != NULL)
        {
            th[i]->join();
            delete th[i];
            th[i] = NULL;
        }
    }
    
    delete[] th;
}

unsigned long int AliasHintCollector::getProbeToken()
{
    unsigned long int token = tokenCounter;
    tokenCounter++;
    return token;
}
