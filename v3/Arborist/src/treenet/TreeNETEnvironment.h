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

#include "../common/thread/Mutex.h"
#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "explorenet/ExploreNETRecord.h"
#include "structure/IPLookUpTable.h"
#include "structure/SubnetSiteSet.h"

class TreeNETEnvironment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Constants to denote the different modes of verbosity (introduced in v3.0)
    const static unsigned short DISPLAY_MODE_LACONIC = 0; // Default
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1;
    const static unsigned short DISPLAY_MODE_VERBOSE = 2;
    const static unsigned short DISPLAY_MODE_DEBUG = 3;

    // Mutex object used when printing out additionnal messages (slightly verbose to debug)
    static Mutex consoleMessagesMutex;

    // Constructor/destructor
    TreeNETEnvironment(ostream *out, 
                       unsigned char startTTL, 
                       unsigned short probingProtocol, 
                       bool exploreLANExplicitly, 
                       bool useLowerBorderAsWell, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
                       bool prescanExpand, 
                       bool saveExploreNETRecords, 
                       InetAddress &localIPAddress, 
                       NetworkAddress &LAN, 
                       string &probeAttentionMessage, 
                       TimeVal &timeoutPeriod, 
                       TimeVal &probeRegulatingPeriod, 
                       TimeVal &probeThreadDelay,
                       unsigned short nbIPIDs, 
                       unsigned short maxRollovers, 
                       double baseTolerance, 
                       double maxError, 
                       unsigned short displayMode, 
                       unsigned short maxThreads);
    ~TreeNETEnvironment();
    
    /*
     * N.B.: there used to be default settings for some of the parameters in the constructor, but 
     * since these default settings are already in Main.cpp, there was no point in keeping them.
     */
    
    // Accessers
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    inline SubnetSiteSet *getIPBlocksToAvoid() { return this->IPBlocksToAvoid; }
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
    
    inline unsigned short getDisplayMode() { return this->displayMode; }
    inline bool debugMode() { return (this->displayMode == DISPLAY_MODE_DEBUG); }
    inline unsigned short getMaxThreads() { return this->maxThreads; }
    
    // Setters (very few of them)
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeoutPeriod = timeout; }
    
    // Methods to handle the ExploreNET records (optional feature, added Aug 29, 2016).
    inline bool savingExploreNETResults() { return this->saveExploreNETRecords; }
    inline void pushExploreNETRecord(ExploreNETRecord *r) { this->xnetRecords.push_back(r); }
    void outputExploreNETRecords(string filename);

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    SubnetSiteSet *IPBlocksToAvoid; // Lists UNDEFINED /20 subnets where expansion should not be done
    
    // Output stream
    ostream *out;
    
    // Various values which stay constant during execution
    unsigned char startTTL;
    unsigned short probingProtocol;
    bool exploreLANExplicitly, useLowerBorderAsWell, doubleProbe, useFixedFlowID, prescanExpand, saveExploreNETRecords;
    InetAddress &localIPAddress;
    NetworkAddress &LAN;
    string &probeAttentionMessage;
    TimeVal &timeoutPeriod, &probeRegulatingPeriod, &probeThreadDelay;
    
    // Extra parameters to calibrate the alias resolution IP ID-based techniques
    unsigned short nbIPIDs;
    unsigned short maxRollovers;
    double baseTolerance;
    double maxError;
    
    /*
     * Value for maintaining display mode; setting it to DISPLAY_MODE_DEBUG (=3) is equivalent to 
     * using the debug mode of ExploreNET. Display modes is a new feature brought by v3.0.
     */
    
    unsigned short displayMode;
    
    // Maximum amount of threads involved during the probing steps
    unsigned short maxThreads;
    
    // Bool value and list for registering the ExploreNET records (optional feature; Aug 29, 2016).
    list<ExploreNETRecord*> xnetRecords;

};

#endif /* TREENETENVIRONMENT_H_ */
