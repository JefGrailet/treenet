/*
 * ProbesDispatcher.h
 *
 *  Created on: Oct 30, 2014
 *      Author: grailet
 *
 * Provided a list of IPs to probe with ICMP, this class is used to dispatch blocks of IPs (of a
 * fixed size) between several threads with their own prober instance in order to speed up the
 * probing of a large amount of IPs (such as a range).
 */

#ifndef PROBESDISPATCHER_H_
#define PROBESDISPATCHER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"

class ProbesDispatcher
{
public:

    static const unsigned short MINIMAL_BLOCK_SIZE = 2;
    static const unsigned short MINIMUM_IPS_PER_THREAD = 2;
    
    enum DispatchResult
    {
        FOUND_ALTERNATIVE,
        FOUND_RESPONSIVE_IPS,
        FOUND_PROOF_TO_DISCARD_ALTERNATIVE,
        FOUND_NOTHING
    };

    // Constructor (args are for the ICMP probers), destructor
    ProbesDispatcher(std::list<InetAddress> IPsToProbe,
                     unsigned char requiredTTL,
                     unsigned char alternativeTTL,
                     unsigned short maxThreads,
                     InetAddress &localIPAddress,
                     string &attentionMessage,
                     bool useFixedFlowID = true,
                     const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD,
                     const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
                     unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                     unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                     unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                     unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE);
    ~ProbesDispatcher();
    
    // Dispatch method
    unsigned short dispatch();
    
    // Accesser to responsive IPs list and "foundAlternative" flag (for probe units)
    inline std::list<InetAddress> *getResponsiveIPs() { return &responsiveIPs; }
    inline bool hasFoundAlternative() { return foundAlternative; }
    inline bool ignoringAlternative() { return ignoreAlternative; }
    
    // Method to set "foundAlternative" and "ignoreAlternative" to true (used by probe units)
    inline void raiseFoundAlternativeFlag() { foundAlternative = true; }
    inline void raiseIgnoreAlternativeFlag() { ignoreAlternative = true; }
    
private:

    // Very own private fields
    std::list<InetAddress> IPsToProbe, responsiveIPs;
    unsigned char requiredTTL, alternativeTTL;
    unsigned short maxThreads;
    
    // Flag telling alternative has been found
    bool foundAlternative;
    
    // Flag telling to not look for alternative anymore
    bool ignoreAlternative;
    
    // Private fields with prober parameters
    InetAddress &localIPAddress;
    string &attentionMessage;
    bool useFixedFlowID;
    const TimeVal &timeoutPeriod, &probeRegulatorPausePeriod;
    unsigned short lowerBoundICMPid, upperBoundICMPid;
    unsigned short lowerBoundICMPseq, upperBoundICMPseq;
};

#endif /* PROBESDISPATCHER_H_ */
