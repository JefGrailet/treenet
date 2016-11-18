/*
 * TreeNETEnvironment.h
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * TreeNETEnvironment is a class which sole purpose is to provide access to structures or 
 * constants (for the current execution, e.g. timeout delay chosen by the user upon starting 
 * TreeNET) which are relevant to the different parts of the program, such that each component 
 * does not have to be passed individually each time a new object is instantiated.
 *
 * While being similar to the class found in "Arborist", the TreeNETEnvironment class in 
 * "Forester" is lighter.
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <ostream>
using std::ostream;

#include "../common/thread/Thread.h"
#include "../common/thread/Mutex.h"
#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "utils/StopException.h" // Not used directly here, but provided to all classes that need it this way
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
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1; // Note: name is for compatibility with classes from Arborist
    const static unsigned short DISPLAY_MODE_DEBUG = 2;

    // Mutex objects used when printing out additionnal messages (slightly verbose to debug) and triggering emergency stop
    static Mutex consoleMessagesMutex;
    static Mutex emergencyStopMutex;

    // Constructor/destructor
    TreeNETEnvironment(ostream *out, 
                       bool usingMerging, 
                       unsigned short probingProtocol, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
                       InetAddress &localIPAddress, 
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
    inline ostream *getOutputStream() { return this->out; }
    
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool usingMergingAtParsing() { return this->usingMerging; }
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
    
    inline unsigned short getDisplayMode() { return this->displayMode; }
    inline bool debugMode() { return (this->displayMode == DISPLAY_MODE_DEBUG); }
    inline unsigned short getMaxThreads() { return this->maxThreads; }
    
    /*
     * (Unique to Forester, for grafting mode) 
     * Methods to copy or transfer the content of a SubnetSiteSet object.
     */
    
    void copySubnetSet(SubnetSiteSet *src);
    void transferSubnetSet(SubnetSiteSet *src);
    
    // Setter for timeout period
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeoutPeriod = timeout; }
    
    /*
     * Method to trigger the (emergency) stop. It is a special method of TreeNETEnvironment which 
     * is meant to force the program to quit when it cannot fully benefit from the host's 
     * resources to conduct measurements/probing. It has a return result (though not used yet), a 
     * boolean one, which is true if the current context successfully triggered the stop 
     * procedure. It will return false it was already triggered by another thread.
     */
    
    bool triggerStop();
    
    // Method to check if the flag for emergency stop is raised.
    inline bool isStopping() { return this->flagEmergencyStop; }

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    
    // Output stream
    ostream *out;
    
    // Various values which stay constant during execution
    bool usingMerging;
    unsigned short probingProtocol;
    bool doubleProbe, useFixedFlowID;
    InetAddress &localIPAddress;
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
    
    // Flag for emergency exit
    bool flagEmergencyStop;

};

#endif /* TREENETENVIRONMENT_H_ */
