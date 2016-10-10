/*
 * TimestampCheckUnit.cpp
 *
 *  Created on: April 13, 2016
 *      Author: grailet
 *
 * Implements the class defined in TimestampCheckUnit.h (see this file to learn more about the 
 * goals of such class).
 */

#include "TimestampCheckUnit.h"
#include "../../common/thread/Thread.h"

TimestampCheckUnit::TimestampCheckUnit(TreeNETEnvironment *e, 
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

    // Instantiates ICMP probing object (necessarily ICMP)
    try
    {
        prober = new DirectICMPProber(env->getAttentionMessage(), 
                                      baseTimeout, 
                                      env->getProbeRegulatingPeriod(), 
                                      lbii, 
                                      ubii, 
                                      lbis, 
                                      ubis, 
                                      env->debugMode());
        
        ((DirectICMPProber*) prober)->useTimestampRequests();
    }
    catch(SocketException e)
    {
        throw e;
    }
}

TimestampCheckUnit::~TimestampCheckUnit()
{
    delete prober;
}

ProbeRecord *TimestampCheckUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    ProbeRecord *record = NULL;
    
    // Never using fixedFlowID in these circumstances
    record = prober->singleProbe(localIP, dst, TTL, false);

    return record;
}

void TimestampCheckUnit::run()
{
    InetAddress target(this->IPToProbe);
    
    ProbeRecord *newProbe = probe(target, PROBE_TTL);
    if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_TS_REPLY)
    {
        // Sometimes, the replying address is not the target: we consider the target does not reply
        if(newProbe->getRplyAddress() == target)
        {
            /*
            ostream *out = env->getOutputStream();
            (*out) << "Replying address: " << newProbe->getRplyAddress() << endl;
            (*out) << "Originate timestamp: " << newProbe->getOriginateTs() << endl;
            (*out) << "Receive timestamp: " << newProbe->getReceiveTs() << endl;
            (*out) << "Transmit timestamp: " << newProbe->getTransmitTs() << endl;
            */
        
            IPLookUpTable *table = env->getIPTable();
            IPTableEntry *entry = table->lookUp(target);
            if(entry != NULL) // Should not occur, but just in case
            {
                entry->setReplyingToTSRequest();
            }
        }
    }
    
    delete newProbe;
}
