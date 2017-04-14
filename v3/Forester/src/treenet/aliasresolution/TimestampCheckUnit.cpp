/*
 * TimestampCheckUnit.cpp
 *
 *  Created on: April 13, 2016
 *      Author: jefgrailet
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
                                       unsigned short ubis):
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
    catch(SocketException &se)
    {
        ostream *out = env->getOutputStream();
        TreeNETEnvironment::consoleMessagesMutex.lock();
        (*out) << "Caught an exception because no new socket could be opened." << endl;
        TreeNETEnvironment::consoleMessagesMutex.unlock();
        this->stop();
        throw;
    }
}

TimestampCheckUnit::~TimestampCheckUnit()
{
    if(prober != NULL)
    {
        env->updateProbeAmounts(prober);
        delete prober;
    }
}

void TimestampCheckUnit::stop()
{
    TreeNETEnvironment::emergencyStopMutex.lock();
    env->triggerStop();
    TreeNETEnvironment::emergencyStopMutex.unlock();
}

ProbeRecord *TimestampCheckUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    ProbeRecord *record = NULL;
    
    // Never using fixedFlowID in these circumstances
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, false);
    }
    catch(SocketException e)
    {
        throw;
    }

    return record;
}

void TimestampCheckUnit::run()
{
    InetAddress target(this->IPToProbe);
    
    ProbeRecord *newProbe = NULL;
    try
    {
        newProbe = this->probe(target, PROBE_TTL);
    }
    catch(SocketException &se)
    {
        this->stop();
        return;
    }
    
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
