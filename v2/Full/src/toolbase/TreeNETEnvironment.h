/*
 * TreeNETEnvironment.h
 *
 *  Created on: Sep 30, 2015
 *      Author: grailet
 *
 * TreeNETEnvironment is a class which sole purpose is to provide access to structures or 
 * constants (for the current execution, e.g. timeout delay chosen by the user upon starting 
 * TreeNET) which are relevant to the different parts of the program, such that each component 
 * does not have to be passed individually each time a new object is instantiated.
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <ostream>
using std::ostream;

#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "./structure/IPLookUpTable.h"
#include "./structure/SubnetSiteSet.h"
#include "./ToolBase.h"

class TreeNETEnvironment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Default values for alias resolution

    // Constructor/destructor
    TreeNETEnvironment(ostream *out, 
                       unsigned char startTTL, 
                       unsigned short probingProtocol, 
                       bool exploreLANExplicitly, 
                       bool useLowerBorderAsWell, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
                       bool prescanExpand, 
                       InetAddress &localIPAddress, 
                       NetworkAddress &LAN, 
                       string &probeAttentionMessage, 
                       TimeVal &timeoutPeriod = ToolBase::DEFAULT_PROBE_TIMEOUT_PERIOD, 
                       TimeVal &probeRegulatingPeriod = ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, 
                       TimeVal &probeThreadDelay = ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, // Fits as well
                       unsigned short nbIPIDs = ToolBase::ALIAS_RESO_DEFAULT_NB_IP_IDS, 
                       unsigned short maxRollovers = ToolBase::ALIAS_RESO_DEFAULT_MAX_ROLLOVERS, 
                       double baseTolerance = ToolBase::ALIAS_RESO_DEFAULT_BASE_TOLERANCE, 
                       double maxError = ToolBase::ALIAS_RESO_DEFAULT_MAX_ERROR, 
                       bool debug = ToolBase::DEFAULT_DEBUG,
                       unsigned short maxThreads = 256);
    ~TreeNETEnvironment();
    
    // Accessers
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    inline ostream *getOutputStream() { return this->out; }
    
    inline unsigned char getStartTTL() { return this->startTTL; }
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool exploringLANExplicitly() { return this->exploreLANExplicitly; }
    inline bool usingLowerBorderAsWell() { return this->useLowerBorderAsWell; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    inline bool expandingAtPrescanning() { return this->prescanExpand; }
    
    inline InetAddress &getLocalIPAddress() { return this->localIPAddress; }
    inline NetworkAddress &getLAN() { return this->LAN; }
    inline string &getAttentionMessage() { return this->probeAttentionMessage; }
    inline TimeVal &getTimeoutPeriod() { return this->timeoutPeriod; }
    inline TimeVal &getProbeRegulatingPeriod() { return this->probeRegulatingPeriod; }
    inline TimeVal &getProbeThreadDelay() { return this->probeThreadDelay; }
    
    inline unsigned short getNbIPIDs() { return this->nbIPIDs; }
    inline unsigned short getMaxRollovers() { return this->maxRollovers; }
    inline double getBaseTolerance() { return this->baseTolerance; }
    inline double getMaxError() { return this->maxError; }
    
    inline bool debugMode() { return this->debug; }
    inline unsigned short getMaxThreads() { return this->maxThreads; }
    
    // Setters (very few of them)
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeoutPeriod = timeout; }

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    
    // Output stream
    ostream *out;
    
    // Various values which stay constant during execution
    unsigned char startTTL;
    unsigned short probingProtocol;
    bool exploreLANExplicitly, useLowerBorderAsWell, doubleProbe, useFixedFlowID, prescanExpand;
    InetAddress &localIPAddress;
    NetworkAddress &LAN;
    string &probeAttentionMessage;
    TimeVal &timeoutPeriod, &probeRegulatingPeriod, &probeThreadDelay;
    
    // Extra parameters to calibrate the alias resolution IP ID-based techniques
    unsigned short nbIPIDs;
    unsigned short maxRollovers;
    double baseTolerance;
    double maxError;
    
    // Boolean for debug/verbose mode in classes originating from ExploreNET
    bool debug;
    
    // Maximum amount of threads involved during the probing steps
    unsigned short maxThreads;

};

#endif /* TREENETENVIRONMENT_H_ */
