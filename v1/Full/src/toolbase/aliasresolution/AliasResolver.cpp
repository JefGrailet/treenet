/*
 * AliasResolver.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * Implements the class defined in AliasResolver.h (see this file to learn further about the 
 * goals of such class).
 */

#include "AliasResolver.h"
#include "IPIDResolverUnit.h"
#include "ReverseDNSResolverUnit.h"
#include "../../common/thread/Thread.h"

AliasResolver::AliasResolver(std::list<InetAddress*> IPs,
                             InetAddress &lIPa,
                             string &msg,
                             bool uffID,
                             const TimeVal &tp,
                             const TimeVal &prpp,
                             unsigned short lbii,
                             unsigned short ubii,
                             unsigned short lbis,
                             unsigned short ubis,
                             unsigned short mt):
IPsToProbe(IPs),
tokenCounter(1),
localIPAddress(lIPa),
attentionMessage(msg),
useFixedFlowID(uffID),
timeoutPeriod(tp),
probeRegulatorPausePeriod(prpp),
lowerBoundICMPid(lbii),
upperBoundICMPid(ubii),
lowerBoundICMPseq(lbis),
upperBoundICMPseq(ubis),
maxThreads(mt)
{
}

AliasResolver::~AliasResolver()
{
}

void AliasResolver::resolve()
{
    unsigned long int nbIPs = (unsigned long int) IPsToProbe.size();
    if(nbIPs == 0)
    {
        return;
    }
    unsigned short nbThreads = 1;
    
    // Computes amount of threads needed
    if(nbIPs > (unsigned long int) maxThreads)
        nbThreads = maxThreads;
    else
        nbThreads = (unsigned short) nbIPs;
    
    // Initializes an array of threads
    Thread **th = new Thread*[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
        th[i] = NULL;

    // Does a copy of the IPs list for host name retrieval (after IP ID retrieval)
    list<InetAddress*> backUpIPsToProbe(IPsToProbe);

    // Starts scheduling for IP ID retrieval
    unsigned short range = (DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    unsigned short j = 0; // For circular movement in the thread array
    for(unsigned long int i = 0; i < nbIPs; i++)
    {
        InetAddress *IPToProbe = IPsToProbe.front();
        IPsToProbe.pop_front();
        
        if(th[j] != NULL)
        {
            th[j]->join();
            delete th[j];
            th[j] = NULL;
        }
        
        th[j] = new Thread(new IPIDResolverUnit(
                this,
                IPToProbe,
                localIPAddress,
                attentionMessage,
                useFixedFlowID,
                timeoutPeriod,
                probeRegulatorPausePeriod,
                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range),
                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range) + range - 1,
                DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
        
        th[j]->start();
        
        j++;
        if(j == maxThreads)
            j = 0;
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
    
    // Now schedules to resolve host names of each IP by reverse DNS
    j = 0;
    for(unsigned long int i = 0; i < nbIPs; i++)
    {
        InetAddress *IPToProbe = backUpIPsToProbe.front();
        backUpIPsToProbe.pop_front();
        
        if(th[j] != NULL)
        {
            th[j]->join();
            delete th[j];
            th[j] = NULL;
        }
        
        th[j] = new Thread(new ReverseDNSResolverUnit(IPToProbe));
        th[j]->start();
        
        j++;
        if(j == maxThreads)
            j = 0;
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

unsigned long int AliasResolver::getProbeToken()
{
    unsigned long int token = tokenCounter;
    tokenCounter++;
    return token;
}
