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

ProbeUnit::ProbeUnit(ProbesDispatcher *p,
                     std::list<InetAddress> IPs,
                     unsigned char rTTL,
                     unsigned char aTTL,
                     InetAddress &lIPa,
                     string &msg,
                     bool uffID,
                     const TimeVal &tp,
                     const TimeVal &prpp,
                     unsigned short lbii,
                     unsigned short ubii,
                     unsigned short lbis,
                     unsigned short ubis) throw(SocketException):
parent(p),
IPsToProbe(IPs),
requiredTTL(rTTL),
alternativeTTL(aTTL),
localIPAddress(lIPa),
useFixedFlowID(uffID)
{
    try
    {
        prober = new DirectICMPProber(msg, tp, prpp, lbii, ubii, lbis, ubis);
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
    ProbeRecord *record = NULL;
    record = prober->singleProbe(this->localIPAddress, dst, TTL, this->useFixedFlowID, false, 0);

    return record;
}

ProbeRecord *ProbeUnit::doubleProbe(const InetAddress &dst, unsigned char TTL)
{
    ProbeRecord *record = NULL;
    record = prober->doubleProbe(this->localIPAddress, dst, TTL, this->useFixedFlowID, false, 0);

    return record;
}

void ProbeUnit::run()
{
    for(std::list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        InetAddress curIP = *it;
        
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
        ProbeRecord *newProbe = doubleProbe(curIP, requiredTTL);
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
            newProbe = doubleProbe(curIP, alternativeTTL);
            
            if(!newProbe->isAnonymousRecord() && newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newProbe;
                
                // Double checks that this IP is not located at alternativeTTL-1
                newProbe = doubleProbe(curIP, alternativeTTL - 1);
                
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
