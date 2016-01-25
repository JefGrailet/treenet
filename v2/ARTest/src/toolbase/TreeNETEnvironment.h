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
 *
 * In the case of the A.R. Assessment version, some parameters have been removed, while others 
 * have been added specifically to calibrate the alias resolution part. Also, there is now a 
 * list of routers.
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <list>
using std::list;
#include <ostream>
using std::ostream;

#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "../common/inet/NetworkAddress.h"
#include "./structure/IPLookUpTable.h"
#include "./structure/Router.h"

class TreeNETEnvironment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Constructor/destructor
    TreeNETEnvironment(ostream *out, 
                       unsigned short probingProtocol, 
                       bool doubleProbe, 
                       bool useFixedFlowID, 
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
                       bool debug = false,
                       unsigned short maxThreads = 256);
    ~TreeNETEnvironment();
    
    // Accessers
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline list<Router*> *getRouterList() { return &(this->routerList); }
    inline ostream *getOutputStream() { return this->out; }
    
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    
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
    
    // (offline mode) List unresponsive interfaces among routers (after IP dictionnary parsing)
    void listUnresponsiveInterfaces();
    
    // Method to output the router list (specific to ARTest)
    void outputRouterList(string fileName);
    
    // Method to output the data for plotting PDF/CDF graphs of success rates (specific to ARTest)
    void outputPlotData(string fileName);

private:

    // Structures
    IPLookUpTable *IPTable;
    list<Router*> routerList;
    
    // Output stream
    ostream *out;
    
    // Various values which stay constant during execution
    unsigned short probingProtocol;
    bool doubleProbe, useFixedFlowID;
    InetAddress &localIPAddress;
    NetworkAddress &LAN;
    string &probeAttentionMessage;
    TimeVal &timeoutPeriod, &probeRegulatingPeriod, &probeThreadDelay;
    
    // Extra parameters to calibrate the alias resolution IP ID-based techniques
    unsigned short nbIPIDs;
    unsigned short maxRollovers;
    double baseTolerance;
    double maxError;
    
    // Boolean for debug/verbose mode in classes originating from ExploreNET (e.g. probers)
    bool debug;
    
    // Maximum amount of threads involved during the probing steps
    unsigned short maxThreads;

};

#endif /* TREENETENVIRONMENT_H_ */
