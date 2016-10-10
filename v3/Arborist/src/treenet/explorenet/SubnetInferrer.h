/*
 * SubnetInferrer.h
 *
 *  Created on: Jul 15, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (october 2014) to improve coding style and study the code
 */

#ifndef SUBNETINFERRER_H_
#define SUBNETINFERRER_H_

#include <sstream>
using std::stringstream;

#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/structure/ProbeRecord.h"
#include "../../prober/exception/SocketException.h"
#include "../../common/date/TimeVal.h"
#include "../../common/inet/InetAddressSet.h"
#include "../utils/ProbeRecordCache.h"
#include "../TreeNETEnvironment.h"
#include "NoTTLEstimationException.h"
#include "UnresponsiveIPException.h"
#include "UndesignatedPivotInterface.h"
#include "SubnetBarrierException.h"
#include "ShortTTLException.h"

class SubnetInferrer
{
public:

    /*
     * Constants. These were originally defined in deprecated class ToolBase; however, as ToolBase 
     * became with time only relevant for SubnetInferrer, its features were merged with it to 
     * remove the ToolBase class from the program for the sake of keeping a clear architecture.
     */
    
    static TimeVal DEFAULT_PROBE_REGULATING_PAUSE_PERIOD;
    static unsigned short DEFAULT_MAX_CONSECUTIVE_ANONYMOUS;
    static unsigned short DEFAULT_MAX_REDUNDANT_IPS;
    static TimeVal DEFAULT_PROBE_TIMEOUT_PERIOD;
    static unsigned int CONJECTURED_GLOBAL_INTERNET_DIAMETER;
    static unsigned char MIN_CORE_IP_SUBNET_PREFIX;
    static unsigned char MIN_CORE_IP_ALIAS_PREFIX;
    static unsigned char DEFAULT_SUBNET_INFERENCE_MIDDLE_TTL;
    static bool DEFAULT_STRICT_POSITIONING;
    static unsigned char DEFAULT_MIDDLE_TTL;
    
    // Constructor and destructor
    SubnetInferrer(TreeNETEnvironment *env, 
                   unsigned short lowerBoundSrcPortORICMPid = DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                   unsigned short upperBoundSrcPortORICMPid = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
                   unsigned short lowerBoundDstPortORICMPseq = DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                   unsigned short upperBoundDstPortICMPseq = DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ) throw (SocketException);
    virtual ~SubnetInferrer();

    /**
     * strictPositioning determines the IP Address range to be explored while looking for a pivot IP address.
     * If strictPositioning is false search space is /29, if it is true the search space is /30
     */
     
    SubnetSite *inferRemoteSubnet(const InetAddress &destinationAddress, 
                                  bool strictSubnetPositioning, 
                                  unsigned char middleTTL, 
                                  bool useLowerBorderAsWell) throw (NoTTLEstimationException, 
                                                                    UnresponsiveIPException, 
                                                                    UndesignatedPivotInterface, 
                                                                    ShortTTLException);
    SubnetSite *inferLocalAreaSubnet(const InetAddress &destinationAddress, 
                                     NetworkAddress &localAreaNetwork); // Contains discovered LAN IP addresses and the subnet mask
    SubnetSite *inferDummyLocalAreaSubnet(const InetAddress &destinationAddress, 
                                          NetworkAddress &localAreaNetwork); // Contains only subnet mask of LAN
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    
    // Method to get the debug/verbose log.
    inline string getExploreNETLog() { return logStream.str(); }

private:

    // TreeNET environment (i.e. provides parameters for probing work)
    TreeNETEnvironment *env;
    
    // Other private fields
    DirectProber *prober;
    ProbeRecordCache cache;
    unsigned int temporaryProbingCost;
    
    // Fields related to the verbose and debug modes
    bool verbose, debug;
    stringstream logStream;
    
    /**
     * Probes a given destination address and records results in 3 distinct ProbeRecord objects.
     * These objects contains the results of 3 different probes targetting the same address, but
     * with distinct TTL. Actually, the method computes siteRecord first and uses the recorded TTL
     * to probe the same destination with TTL - 1 (sitePrevRecord) and TTL - 2 
     * (sitePrevPrevRecord).
     */

    bool populateRecords(const InetAddress &dst,
                         ProbeRecord **siteRecord,
                         ProbeRecord **sitePrevRecord,
                         ProbeRecord **sitePrevPrevRecord,
                         bool strictSubnetPositioning,
                         unsigned char middleTTL,
                         bool useLowerBorderAsWell) throw(NoTTLEstimationException, 
                                                          UnresponsiveIPException, 
                                                          UndesignatedPivotInterface, 
                                                          ShortTTLException);
    
    SubnetSite *exploreSite_B(const InetAddress &dst,
                              ProbeRecord *siteRecord,
                              ProbeRecord *sitePrevRecord,
                              ProbeRecord *sitePrevPrevRecord,
                              bool useLowerBorderAsWell);

    /**
     * Source Based Test
     */
     
    bool discoverAlias_Rule_1_B(const ProbeRecord *sitePrevRecord, const ProbeRecord *newRecorddTTL);
    
    /**
     * 3031 MATE test
     */
     
    bool discoverAlias_Rule_2_B(const ProbeRecord *siteRecord, const InetAddress &currentAddress, unsigned char currentSubnetPrefix);
    
    /**
     * Check if mate-31 or mate-30 of the current address is within the boundaries of the subnet.
     * Basically, PALMTREE idea.
     */
    
    bool discoverAlias_Rule_3_B(const ProbeRecord *sitePrevRecord, 
                                const InetAddress &currentAddress, 
                                unsigned char currentSubnetPrefix, 
                                unsigned char subnetdTTL) throw(SubnetBarrierException);
    /**
     * Distance Based Test with TTL=d-1
     */
    
    bool discoverAlias_Rule_4_B(const ProbeRecord *sitePrevRecord,
                                const ProbeRecord *sitePrevPrevRecord,
                                const InetAddress &currentAddress,
                                unsigned char currentSubnetPrefix,
                                unsigned char subnetdTTL) throw(SubnetBarrierException);
    /**
     * All subnet growing rules must have passed from the first rule
     */
    
    bool growSubnet_Rule_1_B(const ProbeRecord *newRecorddTTLPlus1, unsigned char currentSubnetPrefix) throw(SubnetBarrierException);
    bool growSubnet_Rule_2_B(const ProbeRecord *siteRecord, 
                             const InetAddress &currentAddress, 
                             unsigned char currentSubnetPrefix) throw(SubnetBarrierException);
    
    /**
     * Probes the currentAddress with dTTL, we expect it to return TIME_EXCEEDED unless it is 
     * alias. If it returns an ECHO_REPLY while it is NOT alias stop growing subnet. If the 
     * current address is alias, we conclude that it has passed this test and proceed to the next
     * test. Yet, this test is NOT applicable for dTTL<=1 in that case we assume it has passed the
     * test again. However, there are two cases with respect to prevRecord:
     *         a) if prevRecord is ANONYMOUS then the result of newProbe must be ANONYMOUS
     *         b) if prevRecord is REAL-IP then the TIME_EXCEEDED message must come from the same 
     *         IP of prevRecord
     */

    bool growSubnet_Rule_3_B(const ProbeRecord *sitePrevRecord,
                             const ProbeRecord *newRecorddTTL,
                             unsigned char currentSubnetPrefix) throw(SubnetBarrierException);

    /**
     * (dTTL+1 test of /3130 mate of current address): Get /31 mate of currentAddress and probe 
     * it with dTTL+1. We expect to get an anonymous (not-alive) or ECHO_REPLY, if we get an 
     * ICMP_TIME_EXCEEDED then /31 mate of currentAddress is beyond the subnet hence the 
     * currentAddress.
     */
    
    bool growSubnet_Rule_4_B(const InetAddress &currentAddress, 
                             unsigned char currentSubnetPrefix, 
                             unsigned char subnetdTTL) throw(SubnetBarrierException);

    /**
     * (dTTL test of /3130 mate of current address): Get /31 mate of currentAddress and probe it 
     * with dTTL. We expect not to get an ECHO_REPLY unless it is the alias. We expect to get an 
     * ICMP_TIME_EXCEEDED. If the /31 mate is in use or ANONYMOUS if it is not in use again in 
     * case, it is not alias. If we get an ECHO_REPLY and it is not alias then /31 mate is not 
     * on the subnet hence the currentAddress.
     */
    
    bool growSubnet_Rule_5_B(const InetAddress &currentAddress,
                             const InetAddress &alias, 
                             unsigned char currentSubnetPrefix, 
                             unsigned char subnetdTTL) throw(SubnetBarrierException);
};

#endif /* SUBNETINFERRER_H_ */
