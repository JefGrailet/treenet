/*
 * ExploreNETRunnable.h
 *
 *  Created on: Oct 04, 2014
 *      Author: grailet
 *
 * This class fuses the classes ExploreNETRunnableBox and ExploreNETRunnableSingleInput from 
 * the original ExploreNET (v2.1) into a single thread class. The goal is to redesign this 
 * part of ExploreNET in order to embed it in a larger topology discovery tool (as the 
 * class ExploreNETRunnableMultipleInput is expected to be useless in this context).
 */

#ifndef ExploreNETRunnable_H_
#define ExploreNETRunnable_H_

#include "../../common/thread/Runnable.h"
#include "../../common/thread/Thread.h"
#include "../subnetinference/SubnetInferrer.h"
#include "../../common/date/TimeVal.h"
#include "../../common/inet/InetAddress.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/exception/SocketException.h"
#include "../ToolBase.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/structure/ProbeRecord.h"
#include "../utils/TargetAddress.h"
#include "../structure/SubnetSite.h"
#include "../structure/SubnetSiteSet.h"

class ExploreNETRunnable : public Runnable
{
public:

    // Mutual exclusion object used when writing in the subnet site set...
    static Mutex sssMutex;
    // ... and in the console output (when showInferenceFailures == true)
    static Mutex outMutex;

    // Possible results of ExploreNET
    enum ExploreNETResultType
    {
        SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE,
        SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE, // Contains alive IP addresses of LAN and its subnet mask
        DUMMY_LOCAL_SUBNET_SITE, // Contains only LAN subnet mask
        NULL_SUBNET_SITE,
        UNRESPONSIVE_IP_EXCEPTION,
        UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION,
        SHORT_TTL_EXCEPTION
    };

    // Constructor
    ExploreNETRunnable(SubnetSiteSet *subnetSiteSet,
                       TargetAddress &target,
                       InetAddress &localIPAddress,
                       NetworkAddress &lan,
                       bool exploreLANexplicitly,
                       bool useLowerBorderAsWell,
                       string &probeAttentionMessage,
                       TimeVal &timeoutPeriod = ToolBase::DEFAULT_PROBE_TIMEOUT_PERIOD,
                       TimeVal &probeRegulatingPeriod = ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD,
                       bool doubleProbe = false,
                       bool useFixedFlowID = true,
                       unsigned short lowerBoundSrcPortORICMPid = DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID,
                       unsigned short upperBoundSrcPortORICMPid = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID,
                       unsigned short lowerBoundDstPortORICMPseq = DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                       unsigned short upperBoundDstPortICMPseq = DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ,
                       bool dbg = ToolBase::DEFAULT_DEBUG) throw(SocketException);
    
    // Destructor, run method and print out method
    ~ExploreNETRunnable();
    void run();

    // Accessers/setters
    InetAddress &getLocalIPAddress() { return sinf.getLocalIPAddress(); }
    NetworkAddress &getLocalAreaNetwork() { return this->lan; }
    TargetAddress &getTargetAddress() { return this->target; }
    void setTargetAddress(TargetAddress &target) { this->target = target; }

private:
    SubnetSiteSet *set;
    TargetAddress target;
    bool useLowerBorderAsWell;
    NetworkAddress lan;
    bool exploreLANexplicitly;
    SubnetInferrer sinf;
};

#endif /* ExploreNETRunnable_H_ */
