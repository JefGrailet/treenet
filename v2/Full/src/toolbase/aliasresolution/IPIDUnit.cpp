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
#include "../../common/thread/Thread.h"

Mutex IPIDUnit::collectorMutex(Mutex::ERROR_CHECKING_MUTEX);

IPIDUnit::IPIDUnit(TreeNETEnvironment *e, 
                   AliasHintCollector *gP, 
                   InetAddress IP, 
                   unsigned short lbii, 
                   unsigned short ubii, 
                   unsigned short lbis, 
                   unsigned short ubis) throw (SocketException):
env(e), 
grandParent(gP), 
IPToProbe(IP)
{
    // Default values for results
    this->probeToken = 0;
    this->IPID = 0;
    gettimeofday(&this->timeValue, NULL);
    this->echo = false;
    this->replyTTL = (unsigned char) 0;

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

    // Instantiates probing objects
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
    InetAddress target(this->IPToProbe);
    
    // Tries to get IP ID up to 2 times with increasing timeout
    for(unsigned int nbAttempts = 0; nbAttempts < 2; nbAttempts++)
    {
        // Gets a token
        collectorMutex.lock();
        unsigned long int token = grandParent->getProbeToken();
        collectorMutex.unlock();
        
        // Adapts timeout
        TimeVal currentTimeout = prober->getTimeout();
        TimeVal suggestedTimeout = baseTimeout * (nbAttempts + 1);
        if(suggestedTimeout > currentTimeout)
            prober->setTimeout(suggestedTimeout);
        
        // Performs the probe; gets and saves results and breaks out of current loop if successful
        ProbeRecord *newProbe = probe(target, PROBE_TTL);
        if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY && newProbe->getRplyAddress() == target)
        {
            this->probeToken = token;
            this->IPID = newProbe->getRplyIPidentifier();
            gettimeofday(&this->timeValue, NULL);
            if(newProbe->getSrcIPidentifier() == this->IPID)
                this->echo = true;
            this->replyTTL = newProbe->getRplyTTL();

            delete newProbe;
            break;
        }
        
        delete newProbe;
        
        // Small delay of 0,01s before next probe
        Thread::invokeSleep(TimeVal(0,10000));
    }
}

bool IPIDUnit::hasExploitableResults()
{
    if(this->probeToken != 0)
        return true;
    return false;
}

IPIDTuple *IPIDUnit::getTuple()
{
    IPIDTuple *res = new IPIDTuple(this->probeToken, 
                                   this->IPID, 
                                   this->timeValue,
                                   this->echo, 
                                   this->replyTTL);
    return res;
}
