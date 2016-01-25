/*
 * TreeNETEnvironment.h
 *
 *  Created on: Jan 5, 2016
 *      Author: grailet
 *
 * TreeNETEnvironment is a class which sole purpose is to provide access to structures or 
 * constants (for the current execution, e.g. timeout delay chosen by the user upon starting 
 * TreeNET) which are relevant to the different parts of the program, such that each component 
 * does not have to be passed individually each time a new object is instantiated. This version is 
 * very similar to what can be found in the "full" TreeNET but with (obviously) differences due 
 * to the parameters being different.
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <ostream>
using std::ostream;

#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "../structure/IPLookUpTable.h"
#include "../structure/SubnetSiteSet.h"

class TreeNETEnvironment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Default values (set in TreeNETEnvironment.cpp)
    static TimeVal DEFAULT_PROBE_REGULATING_PAUSE_PERIOD;
    static TimeVal DEFAULT_PROBE_TIMEOUT_PERIOD;
    static bool DEFAULT_DEBUG;
    static unsigned short ALIAS_RESO_DEFAULT_NB_IP_IDS;
    static unsigned short ALIAS_RESO_DEFAULT_MAX_ROLLOVERS;
    static double ALIAS_RESO_DEFAULT_BASE_TOLERANCE;
    static double ALIAS_RESO_DEFAULT_MAX_ERROR;
    
    // Constructor/destructor
    TreeNETEnvironment(ostream *out, 
                       bool setRefinement, 
                       unsigned short probingProtocol, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
                       InetAddress &localIPAddress, 
                       string &probeAttentionMessage, 
                       TimeVal &timeoutPeriod = DEFAULT_PROBE_TIMEOUT_PERIOD, 
                       TimeVal &probeRegulatingPeriod = DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, 
                       TimeVal &probeThreadDelay = DEFAULT_PROBE_REGULATING_PAUSE_PERIOD, // Fits as well
                       unsigned short nbIPIDs = ALIAS_RESO_DEFAULT_NB_IP_IDS, 
                       unsigned short maxRollovers = ALIAS_RESO_DEFAULT_MAX_ROLLOVERS, 
                       double baseTolerance = ALIAS_RESO_DEFAULT_BASE_TOLERANCE, 
                       double maxError = ALIAS_RESO_DEFAULT_MAX_ERROR, 
                       bool debug = DEFAULT_DEBUG, 
                       unsigned short maxThreads = 256);
    ~TreeNETEnvironment();
    
    // Accessers
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    inline ostream *getOutputStream() { return this->out; }
    inline bool getSetRefinement() { return this->setRefinement; }
    
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    
    inline InetAddress &getLocalIPAddress() { return this->localIPAddress; }
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
    
    // Boolean to know if we do merging at each subnet insertion or skip this operation
    bool setRefinement;
    
    // Various values related to probing which stay constant during execution
    unsigned short probingProtocol;
    bool doubleProbe, useFixedFlowID, prescanExpand;
    InetAddress &localIPAddress;
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
