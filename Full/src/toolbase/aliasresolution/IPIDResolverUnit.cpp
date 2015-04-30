/*
 * IPIDResolverUnit.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * Implements the class defined in IPIDResolverUnit.h (see this file to learn further about the 
 * goals of such class).
 */

#include "IPIDResolverUnit.h"

Mutex IPIDResolverUnit::resolverMutex(Mutex::ERROR_CHECKING_MUTEX);

IPIDResolverUnit::IPIDResolverUnit(AliasResolver *p,
                                   InetAddress *IP,
                                   InetAddress &lIPa,
                                   string &msg,
                                   bool uffID,
			                       const TimeVal &tp,
			                       const TimeVal &prpp,
			                       unsigned short lbii,
			                       unsigned short ubii,
			                       unsigned short lbis,
			                       unsigned short ubis) throw (SocketException):
parent(p),
IPToProbe(IP),
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

IPIDResolverUnit::~IPIDResolverUnit()
{
    delete prober;
}

ProbeRecord *IPIDResolverUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    ProbeRecord *record = NULL;
	record = prober->singleProbe(this->localIPAddress, dst, TTL, this->useFixedFlowID, false, 0);

	return record;
}

void IPIDResolverUnit::run()
{
    InetAddress IP(*IPToProbe); // Copy-construction

    // Retries to get IP ID up to 4 times
    unsigned short nbAttempts = 0;
    while(nbAttempts <= 4)
    {
        // Gets a token
        resolverMutex.lock();
        unsigned long int probeToken = parent->getProbeToken();
        resolverMutex.unlock();
        
        // Performs the probe
        ProbeRecord *newProbe = probe(IP, PROBE_TTL);
        if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
	    {
	        IPToProbe->setProbeToken(probeToken);
	        IPToProbe->setIPIdentifier(newProbe->getRplyIPidentifier());
	        
	        // Done: deletes probe record and returns
	        delete newProbe;
	        return;   
	    }
	    
	    // Unsuccessfull: deletes current probe record and increments retry counter
	    delete newProbe;
	    nbAttempts++;
	}
}

