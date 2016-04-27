/*
 * UDPUnreachablePortUnit.cpp
 *
 *  Created on: April 15, 2016
 *      Author: grailet
 *
 * Implements the class defined in UDPUnreachablePortUnit.h (see this file to learn more about the 
 * goals of such class).
 */

#include "UDPUnreachablePortUnit.h"
#include "../../common/thread/Thread.h"

UDPUnreachablePortUnit::UDPUnreachablePortUnit(TreeNETEnvironment *e, 
                                               InetAddress IP, 
                                               unsigned short lbii, 
                                               unsigned short ubii, 
                                               unsigned short lbis, 
                                               unsigned short ubis) throw (SocketException):
env(e), 
IPToProbe(IP)
{
    // Initial timeout
    baseTimeout = env->getTimeoutPeriod();
    
    // If a higher timeout is suggested for this IP, uses it
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *IPEntry = table->lookUp(IPToProbe);
    if(IPEntry != NULL)
    {
        TimeVal suggestedTimeout = IPEntry->getPreferredTimeout();
        if(suggestedTimeout > baseTimeout)
            baseTimeout = suggestedTimeout;
    }

    // Instantiates UDP probing object
    try
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
        
        ((DirectUDPWrappedICMPProber*) prober)->useHighPortNumber();
    }
    catch(SocketException e)
    {
        throw e;
    }
}

UDPUnreachablePortUnit::~UDPUnreachablePortUnit()
{
    delete prober;
}

ProbeRecord *UDPUnreachablePortUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    bool usingFixedFlow = env->usingFixedFlowID();
    
    ProbeRecord *record = NULL;
    record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow, false, 0);

    return record;
}

void UDPUnreachablePortUnit::run()
{
    InetAddress target(this->IPToProbe);
    
    // Tries to get a "Port unreachable error"
    ProbeRecord *newProbe = probe(target, PROBE_TTL);
    
    // We register something only if we get an ICMP code "PORT UNREACHABLE"
    if(newProbe->getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE)
    {
        InetAddress replyingAddress = newProbe->getRplyAddress();
        IPLookUpTable *table = env->getIPTable();
        IPTableEntry *entry = table->lookUp(target);
        if(entry != NULL) // Should not occur, but just in case
        {
            entry->setPortUnreachableSrcIP(replyingAddress);
        }
    }
    
    delete newProbe;
}