/*
 * SubnetRefiner.h
 *
 *  Created on: Oct 23, 2014
 *      Author: grailet
 *
 * This class gathers refinement methods to call on subnets after the scanning (in other words,
 * it is a form of postprocessing). The refinement methods are described just below.
 *
 * 1) Subnet filling
 *
 * During subnet inference, it is rather frequent that some ICMP probes fail while the targetted
 * interfaces are online, leading to missing IPs in a subnet interface list. To palliate this
 * problem, one can reprobe all non-listed IPs that are within the boundaries of the subnet to
 * check if there is some missing interface.
 *
 * 2) Subnet expansion
 *
 * This method consists in overgrowing a subnet which does not list a contra-pivot (i.e. all 
 * listed IPs are at the same hop count, and there must be exactly one IP with hop count - 1,
 * called contra-pivot) and probing all non-listed IPs with a TTL smaller than one hop than 
 * the smallest TTL to get an echo reply from a pivot. Once the contra-pivot is found, the 
 * check stops overgrowth and edits the subnet so that its prefix is the prefix for which the
 * contra-pivot was found (contra-pivot is added in the listed IPs as well). When the method 
 * overgrowths the subnet too much, it stops as well.
 */

#ifndef SUBNETREFINER_H_
#define SUBNETREFINER_H_

#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../structure/SubnetSite.h"
#include "../structure/SubnetSiteSet.h"
#include "ProbesDispatcher.h"

class SubnetRefiner
{
public:

    static const unsigned short LOWEST_PREFIX_ALLOWED = 20;
    static const unsigned short MAX_CONTRAPIVOT_CANDIDATES = 5;

    // Constructor (args beside the set are for the ICMP probers), destructor
    SubnetRefiner(ostream *out,
                  SubnetSiteSet *set,
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
    ~SubnetRefiner();
    
    // Refinement methods
    void expand(SubnetSite *ss);
    void fill(SubnetSite *ss);
    
    /*
     * Expansion method to use for shadow subnets at the end of the scanning ONLY; give them 
     * the greatest possible size without colliding with other (accurate/odd) subnets.
     */
    
    void shadowExpand(SubnetSite *ss);
    
private:
    
    // Output stream
    ostream *out;
    
    // Reference to the set of inferred subnets, to test compatibility of expanded subnets.
    SubnetSiteSet *set;
    
    // Private fields to maintain prober parameters
    InetAddress &localIPAddress;
    string &attentionMessage;
    bool useFixedFlowID;
    const TimeVal &timeoutPeriod, &probeRegulatorPausePeriod;
    unsigned short lowerBoundICMPid, upperBoundICMPid;
    unsigned short lowerBoundICMPseq, upperBoundICMPseq;
    unsigned short maxThreads;
}; 

#endif /* SUBNETREFINER_H_ */
