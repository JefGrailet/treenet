/*
 * ProbeUnit.cpp
 *
 *  Created on: Oct 30, 2014
 *      Author: grailet
 *
 * Implements the class defined in ProbeUnit.h (see this file to learn further about the 
 * goals of such class).
 */

#include "ProbeUnit.h"

Mutex ProbeUnit::dispatcherMutex(Mutex::ERROR_CHECKING_MUTEX);

ProbeUnit::ProbeUnit(TreeNETEnvironment *e, 
                     ProbesDispatcher *p, 
                     std::list<InetAddress> IPs, 
                     unsigned char rTTL, 
                     unsigned char aTTL, 
                     unsigned short lbii, 
                     unsigned short ubii, 
                     unsigned short lbis, 
                     unsigned short ubis) throw(SocketException):
env(e), 
parent(p), 
IPsToProbe(IPs), 
requiredTTL(rTTL), 
alternativeTTL(aTTL)
{
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
                                                    env->getTimeoutPeriod(), 
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
                                                    env->getTimeoutPeriod(), 
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
                                          env->getTimeoutPeriod(), 
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

ProbeUnit::~ProbeUnit()
{
    delete prober;
}

ProbeRecord *ProbeUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    bool usingFixedFlow = env->usingFixedFlowID();
    
    // Changes timeout if necessary for this IP
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *dstEntry = table->lookUp(dst);
    TimeVal preferredTimeout, currentTimeout;
    currentTimeout = prober->getTimeout();
    if(dstEntry != NULL)
        preferredTimeout = dstEntry->getPreferredTimeout();
    
    bool timeoutChanged = false;
    if(preferredTimeout > currentTimeout)
    {
        timeoutChanged = true;
        prober->setTimeout(preferredTimeout);
    }

    ProbeRecord *record = NULL;
    if(env->usingDoubleProbe())
        record = prober->doubleProbe(localIP, dst, TTL, usingFixedFlow, false, 0);
    else
        record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow, false, 0);
    
    // Restores previous timeout value if it was changed
    if(timeoutChanged)
    {
        prober->setTimeout(currentTimeout);
    }

    return record;
}

void ProbeUnit::run()
{
    for(std::list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        InetAddress curIP(*it);
        
        // Checks that alternative probe is needed and that there is no need to stop
        bool performAlternativeProbe = false;
        if(parent->hasFoundAlternative())
        {
            return;
        }
        else if(alternativeTTL > 0 && !parent->ignoringAlternative())
        {
            performAlternativeProbe = true;
        }
    
        // Performs the next probe
        ProbeRecord *newProbe = NULL;
        newProbe = probe(curIP, requiredTTL);
        
        if(!newProbe->isAnonymousRecord() && newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            dispatcherMutex.lock();
            parent->getResponsiveIPs()->push_back(curIP);
            if(!parent->ignoringAlternative())
                parent->raiseIgnoreAlternativeFlag();
            dispatcherMutex.unlock();
        }
        else if(performAlternativeProbe)
        {
            // Reprobes at alternativeTTL and stops if echo reply is received
            delete newProbe;
            
            newProbe = probe(curIP, alternativeTTL);
            
            if(!newProbe->isAnonymousRecord() && newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newProbe;
                
                // Double checks that this IP is not located at alternativeTTL-1
                newProbe = probe(curIP, alternativeTTL - 1);
                
                // Found something before alternative TTL: stops probing with alternativeTTL
                if(!newProbe->isAnonymousRecord() && newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    dispatcherMutex.lock();
                    if(!parent->ignoringAlternative())
                       parent->raiseIgnoreAlternativeFlag();
                    dispatcherMutex.unlock();
                }
                // curIP is indeed at alternativeTTL
                else
                {
                    dispatcherMutex.lock();
                    parent->getResponsiveIPs()->push_front(curIP);
                    if(!parent->hasFoundAlternative())
                        parent->raiseFoundAlternativeFlag();
                    dispatcherMutex.unlock();
                    
                    delete newProbe;
                    return;
                }
            }
        }
        delete newProbe;
    }
}
