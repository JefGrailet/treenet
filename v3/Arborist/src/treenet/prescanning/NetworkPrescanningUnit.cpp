/*
 * NetworkPrescanningUnit.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in NetworkPrescanningUnit.h (see this file to learn further about 
 * the goals of such class).
 */

#include "NetworkPrescanningUnit.h"

Mutex NetworkPrescanningUnit::prescannerMutex(Mutex::ERROR_CHECKING_MUTEX);

NetworkPrescanningUnit::NetworkPrescanningUnit(TreeNETEnvironment *e, 
                                               NetworkPrescanner *p, 
                                               list<InetAddress> IPs, 
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
    catch(SocketException &se)
    {
        ostream *out = env->getOutputStream();
        TreeNETEnvironment::consoleMessagesMutex.lock();
        (*out) << "Caught an exception because no new socket could be opened." << endl;
        TreeNETEnvironment::consoleMessagesMutex.unlock();
        this->stop();
        throw;
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
    {
        env->updateProbeAmounts(prober);
        delete prober;
    }
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
    catch(SocketException &se)
    {
        throw;
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

void NetworkPrescanningUnit::stop()
{
    TreeNETEnvironment::emergencyStopMutex.lock();
    env->triggerStop();
    TreeNETEnvironment::emergencyStopMutex.unlock();
}

void NetworkPrescanningUnit::run()
{
    for(list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        InetAddress curIP = *it;
        
        ProbeRecord *probeRecord = NULL;
        
        try
        {
            probeRecord = probe(curIP);
        }
        catch(SocketException &se)
        {
            this->stop();
            return;
        }
        
        if(probeRecord == NULL)
            continue;
        
        bool responsive = false;
        InetAddress replyingIP = probeRecord->getRplyAddress();
        unsigned char replyType = probeRecord->getRplyICMPtype();
        if(!probeRecord->isAnonymousRecord() && replyType == DirectProber::ICMP_TYPE_ECHO_REPLY && replyingIP == curIP)
        {
            responsive = true;
        }
        
        prescannerMutex.lock();
        parent->callback(curIP, responsive);
        prescannerMutex.unlock();
        
        delete probeRecord;
        
        if(env->isStopping())
            return;
    }
}
