/*
 * NetworkPrescanner.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: grailet
 *
 * Implements the class defined in NetworkPrescanner.h (see this file to learn further about the 
 * goals of such class).
 */

#include <ostream>
using std::ostream;

#include <unistd.h> // For usleep() function

#include "NetworkPrescanner.h"
#include "NetworkPrescanningUnit.h"
#include "../../common/thread/Thread.h"

NetworkPrescanner::NetworkPrescanner(TreeNETEnvironment *env)
{
    this->env = env;
}

NetworkPrescanner::~NetworkPrescanner()
{
}

bool NetworkPrescanner::hasUnresponsiveTargets()
{
    if(unresponsiveTargets.size() > 0)
        return true;
    return false;
}

void NetworkPrescanner::reloadUnresponsiveTargets()
{
    this->targets.clear();
    while(unresponsiveTargets.size() > 0)
    {
        InetAddress cur = unresponsiveTargets.front();
        unresponsiveTargets.pop_front();
        targets.push_back(cur);
    }
}

void NetworkPrescanner::callback(InetAddress target, bool responsive)
{
    /*
     * N.B.: since callback() is called once at a time, it is also used to print out prescanning 
     * results (i.e., which IP is responsive).
     */

    if(!responsive)
    {
        unresponsiveTargets.push_back(target);
        return;
    }
    
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *newEntry = table->create(target);
    if(newEntry != NULL)
    {
        newEntry->setPreferredTimeout(this->timeout);
        
        ostream *out = env->getOutputStream();
        (*out) << target << " is responsive" << endl;
    }
}

void NetworkPrescanner::probe()
{
    /*
     * N.B.: this implementation is very similar to that of ProbesDispatcher, which was defined 
     * last year. However, typing has been updated because unsigned short is way too small when 
     * we want to target large amounts of IPs (for example, entirety of AS224). Unsigned long has 
     * been used instead.
     */
    
    unsigned short maxThreads = env->getMaxThreads();
    unsigned long nbTargets = (unsigned long) targets.size();
    if(nbTargets == 0)
    {
        return;
    }
    unsigned short nbThreads = 1;
    unsigned long targetsPerThread = (unsigned long) NetworkPrescanner::MINIMUM_TARGETS_PER_THREAD;
    unsigned long lastTargets = 0;
    
    // Computes amount of threads and amount of targets being probed per thread
    if(nbTargets > NetworkPrescanner::MINIMUM_TARGETS_PER_THREAD)
    {
        if((nbTargets / targetsPerThread) > (unsigned long) maxThreads)
        {
            unsigned long factor = 2;
            while((nbTargets / (targetsPerThread * factor)) > (unsigned long) maxThreads)
                factor++;
            
            targetsPerThread *= factor;
        }
        
        nbThreads = (unsigned short) (nbTargets / targetsPerThread);
        lastTargets = nbTargets % targetsPerThread;
        if(lastTargets > 0)
            nbThreads++;
    }
    else
        lastTargets = nbTargets;

    // Prepares and launches threads
    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    Thread **th = new Thread*[nbThreads];
    
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        std::list<InetAddress> targetsSubset;
        unsigned short toRead = targetsPerThread;
        if(lastTargets > 0 && i == (nbThreads - 1))
            toRead = lastTargets;
        
        for(unsigned int j = 0; j < toRead; j++)
        {
            InetAddress addr(targets.front());
            targetsSubset.push_back(addr);
            targets.pop_front();
        }

        th[i] = new Thread(new NetworkPrescanningUnit(env, 
                                                      this, 
                                                      targetsSubset, 
                                                      DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                                      DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                                      DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                                      DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
    }

    // Launches thread(s) then waits for completion
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th[i]->start();
        Thread::invokeSleep(env->getProbeThreadDelay());
    }
    
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th[i]->join();
        delete th[i];
    }
    
    delete[] th;
}
