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
#include "IPIDUnit.h"
#include "../../common/thread/Thread.h"

AliasHintCollector::AliasHintCollector(TreeNETEnvironment *env)
{
    this->env = env;
    tokenCounter = 1;
}

AliasHintCollector::~AliasHintCollector()
{
}

void AliasHintCollector::setIPsToProbe(Router *router)
{
    IPLookUpTable *table = env->getIPTable();
    list<InetAddress> *expected = router->getExpectedIPs();
    for(list<InetAddress>::iterator it = expected->begin(); it != expected->end(); ++it)
    {
        // Filters out IPs which were unresponsive (so, absent from the IP table)
        IPTableEntry *entry = table->lookUp((*it));
        if(entry == NULL)
        {
            router->addUnresponsiveIP((*it));
            continue;
        }
    
        InetAddress copy((*it));
        IPsToProbe.push_back(copy);
    }
}

void AliasHintCollector::collect()
{
    unsigned short maxThreads = env->getMaxThreads();
    unsigned short nbIPs = 0;
    unsigned short nbThreads = 1;
    
    // Computes amount of IPs to probe
    nbIPs = (unsigned short) IPsToProbe.size();
    if(nbIPs == 0)
    {
        return;
    }
    
    // Computes amount of threads needed
    if(nbIPs > (unsigned long int) maxThreads)
        nbThreads = maxThreads;
    else
        nbThreads = (unsigned short) nbIPs;
    
    // Initializes an array of threads
    Thread **th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;

    // Starts scheduling for IP ID retrieval
    unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    unsigned short j = 0; // For circular movement in the thread array
    for(unsigned long int i = 0; i < nbIPs; i++)
    {
        InetAddress IPToProbe = IPsToProbe.front();
        IPsToProbe.pop_front();
        
        if(th[j] != NULL)
        {
            th[j]->join();
            delete th[j];
            th[j] = NULL;
        }
        
        th[j] = new Thread(new IPIDUnit(env, 
                                        this, 
                                        IPToProbe, 
                                        DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range), 
                                        DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range) + range - 1, 
                                        DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                        DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
        
        th[j]->start();
        
        j++;
        if(j == maxThreads)
            j = 0;
        
        // 0,01s of delay before next thread
        Thread::invokeSleep(TimeVal(0, 10000));
    }

    // No reverse DNS here

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
