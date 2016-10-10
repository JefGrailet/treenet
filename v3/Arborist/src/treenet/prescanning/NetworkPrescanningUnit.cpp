/*
 * NetworkPrescanningUnit.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: grailet
 *
 * Implements the class defined in NetworkPrescanningUnit.h (see this file to learn further about 
 * the goals of such class).
 */

#include "NetworkPrescanningUnit.h"

Mutex NetworkPrescanningUnit::prescannerMutex(Mutex::ERROR_CHECKING_MUTEX);

NetworkPrescanningUnit::NetworkPrescanningUnit(TreeNETEnvironment *e, 
                                               NetworkPrescanner *p, 
                                               std::list<InetAddress> IPs, 
                                               unsigned short lbii, 
                                               unsigned short ubii, 
                                               unsigned short lbis, 
                                               unsigned short ubis) throw(SocketException):
env(e), 
parent(p), 
IPsToProbe(IPs)
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
                                                    p->getTimeoutPeriod(), 
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
                                                    p->getTimeoutPeriod(), 
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
                                          p->getTimeoutPeriod(), 
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
    
    if(env->debugMode())
    {
        TreeNETEnvironment::consoleMessagesMutex.lock();
        ostream *out = env->getOutputStream();
        (*out) << prober->getAndClearLog();
        TreeNETEnvironment::consoleMessagesMutex.unlock();
    }
}

NetworkPrescanningUnit::~NetworkPrescanningUnit()
{
    if(prober != NULL)
        delete prober;
}

ProbeRecord *NetworkPrescanningUnit::probe(const InetAddress &dst)
{
    InetAddress localIP = env->getLocalIPAddress();
    bool usingFixedFlow = env->usingFixedFlowID();
    unsigned char TTL = VIRTUALLY_INFINITE_TTL;

    ProbeRecord *record = NULL;
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow);
    }
    // Just in case
    catch(SocketSendException e)
    {
        record = NULL;
    }
    
    // Debug log
    if(env->debugMode())
    {
        TreeNETEnvironment::consoleMessagesMutex.lock();
        ostream *out = env->getOutputStream();
        (*out) << prober->getAndClearLog();
        TreeNETEnvironment::consoleMessagesMutex.unlock();
    }

    return record;
}

void NetworkPrescanningUnit::run()
{
    for(std::list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        InetAddress curIP = *it;
        
        ProbeRecord *probeRecord = probe(curIP);
        
        if(probeRecord == NULL)
            continue;
        
        bool responsive = false;
        if(!probeRecord->isAnonymousRecord() && probeRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            responsive = true;
        }
        
        prescannerMutex.lock();
        parent->callback(curIP, responsive);
        prescannerMutex.unlock();
        
        delete probeRecord;
    }
}
