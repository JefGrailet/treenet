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
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <ostream>
using std::ostream;
#include <fstream>
using std::ofstream;

#include "../common/thread/Mutex.h"
#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "../prober/DirectProber.h"
#include "scanning/explorenet/ExploreNETRecord.h"
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
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1;
    const static unsigned short DISPLAY_MODE_VERBOSE = 2;
    const static unsigned short DISPLAY_MODE_DEBUG = 3;

    // Mutex objects used when printing out additionnal messages (slightly verbose to debug) and triggering emergency stop
    static Mutex consoleMessagesMutex;
    static Mutex emergencyStopMutex;

    // Constructor/destructor
    TreeNETEnvironment(ostream *consoleOut, 
                       bool externalLogs, 
                       unsigned char startTTL, 
                       unsigned short probingProtocol, 
                       bool exploreLANExplicitly, 
                       bool useLowerBorderAsWell, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
                       bool prescanExpand, 
                       bool prescanThirdOpinion, 
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
    
    // Accesser to the output stream is not inline, because it depends of the settings
    ostream *getOutputStream();
    inline bool usingExternalLogs() { return this->externalLogs; }
    
    inline unsigned char getStartTTL() { return this->startTTL; }
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool exploringLANExplicitly() { return this->exploreLANExplicitly; }
    inline bool usingLowerBorderAsWell() { return this->useLowerBorderAsWell; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    inline bool expandingAtPrescanning() { return this->prescanExpand; }
    inline bool usingPrescanningThirdOpinion() { return this->prescanThirdOpinion; }
    
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
    
    // Methods to handle total amounts of (successful) probes
    void updateProbeAmounts(DirectProber *proberObject);
    void resetProbeAmounts();
    inline unsigned int getTotalProbes() { return this->totalProbes; }
    inline unsigned int getTotalSuccessfulProbes() { return this->totalSuccessfulProbes; }
    
    // Method to handle the output stream writing in an output file.
    void openLogStream(string filename, bool message = true);
    void closeLogStream();
    
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
    
    /*
     * (March 2017) Method to fill the IP dictionnary with IPs found in the routes to the subnets 
     * (when they are missing). Originally, the fill would occur at the alias hints collection, 
     * but it was moved here in the next method for two reasons:
     * 1) Convenience (because access to both subnets and IP dictionnary here).
     * 2) IPs exclusively in routes should recorded as soon as possible to study route stretching.
     */
    
    void recordRouteStepsInDictionnary();

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    SubnetSiteSet *IPBlocksToAvoid; // Lists UNDEFINED /20 subnets where expansion should not be done
    
    /*
     * Output streams (main console output and file stream for the external logs). Having both is 
     * useful because the console output stream will still be used to advertise the creation of a 
     * new log file and emergency stop if the user requested probing details to be written in 
     * external logs.
     */
    
    ostream *consoleOut;
    ofstream logStream;
    bool externalLogs, isExternalLogOpened;
    
    // Various values which stay constant during execution
    unsigned char startTTL;
    unsigned short probingProtocol;
    bool exploreLANExplicitly, useLowerBorderAsWell, doubleProbe, useFixedFlowID;
    bool prescanExpand, prescanThirdOpinion, saveExploreNETRecords;
    InetAddress &localIPAddress;
    NetworkAddress &LAN;
    string &probeAttentionMessage;
    TimeVal &timeoutPeriod, &probeRegulatingPeriod, &probeThreadDelay;
    
    // Extra parameters to calibrate the alias resolution IP ID-based techniques
    unsigned short nbIPIDs;
    unsigned short maxRollovers;
    double baseTolerance;
    double maxError;
    
    // Field for maintaining display mode (max. value = 3, amounts to debug mode)
    unsigned short displayMode;
    
    // Maximum amount of threads involved during the probing steps
    unsigned short maxThreads;
    
    // List for registering the ExploreNET records (optional feature)
    list<ExploreNETRecord*> xnetRecords;
    
    // Fields to record the amount of (successful) probes used during some stage (can be reset)
    unsigned int totalProbes;
    unsigned int totalSuccessfulProbes;
    
    // Flag for emergency exit
    bool flagEmergencyStop;

};

#endif /* TREENETENVIRONMENT_H_ */
