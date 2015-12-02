/*
 * ExploreNETEnvironment.h
 *
 *  Created on: Nov 2, 2015
 *      Author: grailet
 *
 * ExploreNETEnvironment is a class analog to TreeNETEnvironment in the full TreeNET, but 
 * simplified due to the fact that ExploreNET++ should only perform what ExploreNET v2.1 can do. 
 * For example, this class does not have an IPLookUpTable private object (prescanning is exclusive 
 * to TreeNET).
 */

#ifndef EXPLORENETENVIRONMENT_H_
#define EXPLORENETENVIRONMENT_H_

#include <ostream>
using std::ostream;
#include <list>
using std::list;

#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "./structure/SubnetSiteSet.h"
#include "./ToolBase.h"

class ExploreNETEnvironment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Constructor/destructor
    ExploreNETEnvironment(ostream *out, 
                          unsigned char startTTL, 
                          unsigned short probingProtocol, 
                          bool exploreLANExplicitly, 
                          bool useLowerBorderAsWell, 
                          bool doubleProbe, 
                          bool useFixedFlowID, 
                          InetAddress &localIPAddress, 
                          NetworkAddress &LAN, 
                          string &probeAttentionMessage, 
                          TimeVal &timeoutPeriod = ToolBase::DEFAULT_PROBE_TIMEOUT_PERIOD, 
                          TimeVal &probeRegulatingPeriod = ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, 
                          TimeVal &probeThreadDelay = ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, // Fits as well
                          bool debug = ToolBase::DEFAULT_DEBUG,
                          unsigned short maxThreads = 256);
    ~ExploreNETEnvironment();
    
    // Accessers
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    inline ostream *getOutputStream() { return this->out; }
    
    inline unsigned char getStartTTL() { return this->startTTL; }
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool exploringLANExplicitly() { return this->exploreLANExplicitly; }
    inline bool usingLowerBorderAsWell() { return this->useLowerBorderAsWell; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    
    inline InetAddress &getLocalIPAddress() { return this->localIPAddress; }
    inline NetworkAddress &getLAN() { return this->LAN; }
    inline string &getAttentionMessage() { return this->probeAttentionMessage; }
    inline TimeVal &getTimeoutPeriod() { return this->timeoutPeriod; }
    inline TimeVal &getProbeRegulatingPeriod() { return this->probeRegulatingPeriod; }
    inline TimeVal &getProbeThreadDelay() { return this->probeThreadDelay; }
    
    inline bool debugMode() { return this->debug; }
    inline unsigned short getMaxThreads() { return this->maxThreads; }
    
    // Setters (very few of them)
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeoutPeriod = timeout; }

private:

    // Subnet set (simplified version)
    SubnetSiteSet *subnetSet;

    // Output stream
    ostream *out;
    
    // Various values which stay constant during execution
    unsigned char startTTL;
    unsigned short probingProtocol;
    bool exploreLANExplicitly, useLowerBorderAsWell, doubleProbe, useFixedFlowID;
    InetAddress &localIPAddress;
    NetworkAddress &LAN;
    string &probeAttentionMessage;
    TimeVal &timeoutPeriod, &probeRegulatingPeriod, &probeThreadDelay;
    
    // Boolean for debug/verbose mode in classes originating from ExploreNET
    bool debug;
    
    // Maximum amount of threads involved during the probing steps
    unsigned short maxThreads;

};

#endif /* EXPLORENETENVIRONMENT_H_ */
