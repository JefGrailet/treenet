/*
 * IPIDUnit.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * Implements the class defined in IPIDUnit.h (see this file to learn further about the goals of 
 * such class).
 */

#include "IPIDUnit.h"
#include <ctime> // For clock()
#include "../../common/thread/Thread.h"

Mutex IPIDUnit::collectorMutex(Mutex::ERROR_CHECKING_MUTEX);

IPIDUnit::IPIDUnit(TreeNETEnvironment *e, 
                   AliasHintCollector *p, 
                   InetAddress &IP, 
                   unsigned short lbii, 
                   unsigned short ubii, 
                   unsigned short lbis, 
                   unsigned short ubis) throw (SocketException):
env(e), 
parent(p), 
IPToProbe(IP)
{
    baseTimeout = env->getTimeoutPeriod();
    
    // If a higher timeout is suggested for this IP, uses it
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *IPEntry = table->lookUp(IPToProbe);
    if(IPEntry != NULL) // Just in case (but should not occur)
    {
        TimeVal suggestedTimeout = IPEntry->getPreferredTimeout();
        if(suggestedTimeout > baseTimeout)
            baseTimeout = suggestedTimeout;
    }

    try
    {
        unsigned short protocol = env->getProbingProtocol();
        
        if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_UDP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env->usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectUDPWrappedICMPProber(env->getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    baseTimeout, 
                                                    env->getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env->debugMode());
        }
        else if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_TCP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env->usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectTCPWrappedICMPProber(env->getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    baseTimeout, 
                                                    env->getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env->debugMode());
        }
        else
        {
            prober = new DirectICMPProber(env->getAttentionMessage(), 
                                          baseTimeout, 
                                          env->getProbeRegulatingPeriod(), 
                                          lbii, 
                                          ubii, 
                                          lbis, 
                                          ubis, 
                                          env->debugMode());
        }
    }
    catch(SocketException e)
    {
        throw e;
    }
}

IPIDUnit::~IPIDUnit()
{
    delete prober;
}

ProbeRecord *IPIDUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    bool usingFixedFlow = env->usingFixedFlowID();

    ProbeRecord *record = NULL;
    record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow, false, 0);

    return record;
}

void IPIDUnit::run()
{
    // Finds entry in IP table
    InetAddress target = this->IPToProbe;
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *IPEntry = table->lookUp(target);
    
    // Just in case (normally, should not occur, but we never know with PlanetLab)
    if(IPEntry == NULL)
        return;
    
    /*
     * Now, #nbIPIDs different IP IDs are being collected. The time between each IP ID is measured 
     * and saved afterwards. Probe tokens help to "locate" each probe on a timeline, which can be 
     * useful for straightforward Ally (i.e., if for 2 addresses x, y we got the pairs 
     * [t_0,x_id_0], [t_1,y_id_0] and [t_2,x_id_1] with t_0 < t_1 < t_2 and x_id_0 < y_id_0 < 
     * x_id_1 and x_id_1 - y_id_0 < some threshold, those IPs belong to the same router).
     */
     
    timeval startTime;
    bool startSet = false;
    unsigned short nbIPIDs = env->getNbIPIDs();
    
    for(unsigned short i = 0; i < nbIPIDs; i++)
    {
        bool success = false;
        
        // Tries to get IP ID up to 3 times with increasing timeout
        for(unsigned int nbAttempts = 0; nbAttempts < 3; nbAttempts++)
        {
            // Gets a token
            collectorMutex.lock();
            unsigned long int probeToken = parent->getProbeToken();
            collectorMutex.unlock();
            
            // Adapts timeout
            TimeVal currentTimeout = prober->getTimeout();
            TimeVal suggestedTimeout = baseTimeout * (nbAttempts + 1);
            if(suggestedTimeout > currentTimeout)
                prober->setTimeout(suggestedTimeout);
            
            // Performs the probe; gets delay and breaks out of current loop if successful
            ProbeRecord *newProbe = probe(target, PROBE_TTL);
            if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY && newProbe->getRplyAddress() == target)
            {
                IPEntry->setProbeToken(i, probeToken);
                IPEntry->setIPIdentifier(i, newProbe->getRplyIPidentifier());
                
                if(startSet)
                {
                    timeval endTime;
                    gettimeofday(&endTime, NULL);
                    unsigned long seconds, useconds;
                    
                    seconds  = endTime.tv_sec  - startTime.tv_sec;
                    useconds = endTime.tv_usec - startTime.tv_usec;
                    
                    unsigned long delay = seconds + useconds;
                    IPEntry->setDelay(i - 1, delay);
                }
                gettimeofday(&startTime, NULL);
                startSet = true;
                
                delete newProbe;
                success = true;
                break;
            }
            
            delete newProbe;
        }
        
        // Small delay of 0,01s before next probe
        Thread::invokeSleep(TimeVal(0,10000));
        
        // If we cannot get an IP ID at this point, there is not point in continuing
        if(!success)
        {
            break;
        }
    }
}
