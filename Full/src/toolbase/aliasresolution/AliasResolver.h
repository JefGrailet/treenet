/*
 * AliasResolver.h
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * Provided a list of IPs to probe with ICMP, this class spawns a series a thread, each of them 
 * probing a single IP and retrieving the IP identifier found in the response. This IP is later 
 * written inside the InetAddress object which corresponds to the probed IP. AliasResolver also
 * associates a "probe token" to each IP, which is a number taken from a single counter 
 * (synchronized with a Mutex object), which is later used to locate the IP identifiers in time.
 *
 * This duo IP identifier/probe token is later used to resolve aliases in a neighborhood (obtained 
 * in the network tree), therefore allowing the inference of routers within a neighborhood. In a 
 * sense, the name of the class is misleading, because it only performs the probing part of alias
 * resolution. The resolution itself is implemented in NetworkTreeNode, but relies on the data 
 * collected by AliasResolver.
 *
 * N.B.: here, the TTL of each probe is a constant. Indeed, TTL is not relevant for these new
 * probes. This constant is defined by the class AliasResolverUnit.
 */

#ifndef ALIASRESOLVER_H_
#define ALIASRESOLVER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"

class AliasResolver
{
public:

    // Constructor (args are for the ICMP probers), destructor
    AliasResolver(std::list<InetAddress*> IPsToProbe, 
                  InetAddress &localIPAddress,
                  string &attentionMessage,
			      bool useFixedFlowID = true,
			      const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD,
			      const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
			      unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
			      unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
			      unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
			      unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE,
			      unsigned short maxThreads = 16);
	~AliasResolver();
	
	// Method to start the probing
	void resolve();
	
	// Method to get a token (used by AliasResolverUnit)
	unsigned long int getProbeToken();
	
private:

    // Very own private fields
    std::list<InetAddress*> IPsToProbe;
    unsigned long int tokenCounter;
    
    // Private fields with prober parameters + maximum amount of threads running at the same time
    InetAddress &localIPAddress;
    string &attentionMessage;
	bool useFixedFlowID;
	const TimeVal &timeoutPeriod, &probeRegulatorPausePeriod;
    unsigned short lowerBoundICMPid, upperBoundICMPid;
    unsigned short lowerBoundICMPseq, upperBoundICMPseq;
    unsigned short maxThreads;
};

#endif /* ALIASRESOLVER_H_ */

