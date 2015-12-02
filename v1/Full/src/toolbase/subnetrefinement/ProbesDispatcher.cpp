/*
 * ProbesDispatcher.cpp
 *
 *  Created on: Oct 30, 2014
 *      Author: grailet
 *
 * Implements the class defined in ProbesDispatcher.h (see this file to learn further about the 
 * goals of such class).
 */

#include "ProbesDispatcher.h"
#include "ProbeUnit.h"
#include "../../common/thread/Thread.h"

ProbesDispatcher::ProbesDispatcher(std::list<InetAddress> IPs,
                                   unsigned char rTTL,
                                   unsigned char aTTL,
                                   unsigned short mt,
                                   InetAddress &lIPa,
                                   string &msg,
                                   bool uffID,
                                   const TimeVal &tp,
                                   const TimeVal &prpp,
                                   unsigned short lbii,
                                   unsigned short ubii,
                                   unsigned short lbis,
                                   unsigned short ubis):
IPsToProbe(IPs),
requiredTTL(rTTL),
alternativeTTL(aTTL),
maxThreads(mt),
localIPAddress(lIPa),
attentionMessage(msg),
useFixedFlowID(uffID),
timeoutPeriod(tp),
probeRegulatorPausePeriod(prpp),
lowerBoundICMPid(lbii),
upperBoundICMPid(ubii),
lowerBoundICMPseq(lbis),
upperBoundICMPseq(ubis)
{
    foundAlternative = false;
    ignoreAlternative = false;
}

ProbesDispatcher::~ProbesDispatcher()
{
}

unsigned short ProbesDispatcher::dispatch()
{
    unsigned short nbIPs = (unsigned short) IPsToProbe.size();
    if(nbIPs == 0)
    {
        return ProbesDispatcher::FOUND_NOTHING;
    }
    unsigned short nbThreads = 1;
    unsigned short IPsPerThread = ProbesDispatcher::MINIMAL_BLOCK_SIZE;
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

    unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        std::list<InetAddress> IPsSubset;
        unsigned short toRead = IPsPerThread;
        if(lastIPs > 0 && i == (nbThreads - 1))
            toRead = lastIPs;
        
        for(unsigned int j = 0; j < toRead; j++)
        {
            InetAddress addr = IPsToProbe.front();
            IPsSubset.push_back(addr);
            IPsToProbe.pop_front();
        }
        
        th[i] = new Thread(new ProbeUnit(
                this,
                IPsSubset,
                requiredTTL,
                alternativeTTL,
                localIPAddress,
                attentionMessage,
                useFixedFlowID,
                timeoutPeriod,
                probeRegulatorPausePeriod,
                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range),
                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1,
                DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
    }

    // Launches thread(s) then waits for completion
    for(unsigned int i = 0; i < nbThreads; i++)
        th[i]->start();
    
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th[i]->join();
        delete th[i];
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
