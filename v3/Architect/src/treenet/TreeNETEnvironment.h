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
 * In "Architect", the TreeNETEnvironment class is much less relevant due to the passivity of 
 * the tool. In particular, there is no "emergency stop" feature and no longer Mutex objects 
 * (as there is no concurrency here).
 */

#ifndef TREENETENVIRONMENT_H_
#define TREENETENVIRONMENT_H_

#include <ostream>
using std::ostream;
#include <fstream>
using std::ofstream;

#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "structure/IPLookUpTable.h"
#include "structure/SubnetSiteSet.h"

class TreeNETEnvironment
{
public:

    // Constants to denote the different modes of verbosity (introduced in v3.0)
    const static unsigned short DISPLAY_MODE_LACONIC = 0; // Default
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1; // Note: name is for compatibility with classes from Arborist
    const static unsigned short DISPLAY_MODE_DEBUG = 2;

    // Constructor/destructor
    TreeNETEnvironment(ostream *consoleOut, 
                       bool usingMerging, 
                       unsigned short nbIPIDs, 
                       unsigned short maxRollovers, 
                       double baseTolerance, 
                       double maxError, 
                       unsigned short displayMode);
    ~TreeNETEnvironment();
    
    /*
     * N.B.: there used to be default settings for some of the parameters in the constructor, but 
     * since these default settings are already in Main.cpp, there was no point in keeping them.
     */
    
    // Accessers
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    
    // Accesser to the output stream is not inline, because it depends of the settings
    ostream *getOutputStream();
    
    inline bool usingMergingAtParsing() { return this->usingMerging; }
    
    inline unsigned short getNbIPIDs() { return this->nbIPIDs; }
    inline unsigned short getMaxRollovers() { return this->maxRollovers; }
    inline double getBaseTolerance() { return this->baseTolerance; }
    inline double getMaxError() { return this->maxError; }
    
    inline unsigned short getDisplayMode() { return this->displayMode; }
    inline bool debugMode() { return (this->displayMode == DISPLAY_MODE_DEBUG); }
    
    // Method to handle the output stream writing in an output file.
    void openLogStream(string filename);
    void closeLogStream();
    
    /*
     * March 2017: special method to pass through the subnet set and copy all mentioned interfaces 
     * into the IP dictionnary. The goal is to mitigate the absence of a valid .ip file.
     */
    
    void fillIPDictionnary();

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    
    // Output stream
    ostream *consoleOut;
    ofstream logStream;
    bool isExternalLogOpened;
    
    // Various values which stay constant during execution
    bool usingMerging;
    
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

};

#endif /* TREENETENVIRONMENT_H_ */
