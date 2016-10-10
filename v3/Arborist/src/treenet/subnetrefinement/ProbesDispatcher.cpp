/*
 * ProbesDispatcher.cpp
 *
 *  Created on: Oct 30, 2014
 *      Author: grailet
 *
 * Implements the class defined in ProbesDispatcher.h (see this file to learn further about the 
 * goals of such class).
 */

#include <unistd.h> // For usleep() function

#include "ProbesDispatcher.h"
#include "ProbeUnit.h"
#include "../../common/thread/Thread.h"

ProbesDispatcher::ProbesDispatcher(TreeNETEnvironment *e, 
                                   std::list<InetAddress> IPs, 
                                   unsigned char rTTL, 
                                   unsigned char aTTL):
env(e), 
IPsToProbe(IPs), 
requiredTTL(rTTL), 
alternativeTTL(aTTL)
{
    foundAlternative = false;
    ignoreAlternative = false;
}

ProbesDispatcher::~ProbesDispatcher()
{
}

unsigned short ProbesDispatcher::dispatch()
{
    /*
     * N.B.: the "unsigned short" typing is enough for the refinement, because we assume that 
     * there is no subnet larger than /20 (at least, it hasn't been observed yet). However, to 
     * use a similar reasoning for other applications, consider changing the typing (for example, 
     * NetworkPrescanner class uses a similar code but with "unsigned long").
     */

    unsigned short maxThreads = env->getMaxThreads();
    unsigned short nbIPs = (unsigned short) IPsToProbe.size();
    if(nbIPs == 0)
    {
        return ProbesDispatcher::FOUND_NOTHING;
    }
    unsigned short nbThreads = 1;
    unsigned short IPsPerThread = ProbesDispatcher::MINIMUM_IPS_PER_THREAD;
    unsigned short lastIPs = 0;
    
    // Computes amount of threads and amount of IPs being probed per thread
    if(nbIPs > ProbesDispatcher::MINIMUM_IPS_PER_THREAD)
    {
        if((nbIPs / IPsPerThread) > maxThreads)
        {
            unsigned short factor = 2;
            while((nbIPs / (IPsPerThread * factor)) > maxThreads)
                factor++;
            
            IPsPerThread *= factor;
        }
        
        nbThreads = nbIPs / IPsPerThread;
        lastIPs = nbIPs % IPsPerThread;
        if(lastIPs > 0)
            nbThreads++;
    }
    else
        lastIPs = nbIPs;
    
    // Creates #nbThreads thread(s) and give #IPsPerThread IPs to each of them
    Thread **th = new Thread*[nbThreads];

    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        std::list<InetAddress> IPsSubset;
        unsigned short toRead = IPsPerThread;
        if(lastIPs > 0 && i == (nbThreads - 1))
            toRead = lastIPs;
        
        for(unsigned int j = 0; j < toRead; j++)
        {
            InetAddress addr(IPsToProbe.front());
            IPsSubset.push_back(addr);
            IPsToProbe.pop_front();
        }
        
        th[i] = new Thread(new ProbeUnit(env, 
                                         this, 
                                         IPsSubset, 
                                         requiredTTL, 
                                         alternativeTTL, 
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
        if(env->debugMode())
        {
            // No mutex needed because we treat the ProbeUnit objects in order.
            ostream *out = env->getOutputStream();
            (*out) << ((ProbeUnit*) th[i]->getRunnable())->getLog();
        }
        delete th[i];
    }
    
    /*
     * Additionnal line break in debug mode to ensure harmonious display, because there should 
     * be a single line break after the logs and before the next messages from SubnetRefiner.
     */
    
    if(env->debugMode())
    {
        ostream *out = env->getOutputStream();
        (*out) << endl;
    }
    
    delete[] th;
    
    // Computes value to return
    unsigned short result = ProbesDispatcher::FOUND_NOTHING;
    if(foundAlternative)
    {
        result = ProbesDispatcher::FOUND_ALTERNATIVE;
    }
    else if(responsiveIPs.size() > 0)
    {
        result = ProbesDispatcher::FOUND_RESPONSIVE_IPS;
    }
    else if(responsiveIPs.size() == 0 && ignoreAlternative)
    {
        result = ProbesDispatcher::FOUND_PROOF_TO_DISCARD_ALTERNATIVE;
    }

    return result;
}
