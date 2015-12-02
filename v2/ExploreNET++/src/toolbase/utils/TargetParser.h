/*
 * TargetParser.h
 *
 *  Created on: Oct 2, 2015
 *      Author: grailet
 *
 * This class is dedicated to parsing the target IPs and IP blocks which are fed to TreeNET at 
 * launch. Initially, the whole code was found in Main.cpp, but its size and the need to handle 
 * the listing of pre-scan targets in addition to the usual targets led to the creation of this 
 * class. In addition to parsing, this class also handles the target re-ordering.
 */
 
#ifndef TARGETPARSER_H_
#define TARGETPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "../ExploreNETEnvironment.h"
#include "../../common/inet/NetworkAddress.h"

class TargetParser
{
public:

    // Constructor, destructor
    TargetParser(ExploreNETEnvironment *env);
    ~TargetParser();
    
    /*
     * Parsing methods: one for targets from command-line, one for targets found in an input file. 
     * The parsed results are stored in the TargetParser object instead of being returned.
     */
    
    void parseCommandLine(string targetListStr);
    void parseInputFile(string inputFileContent);
    
    // Accessers to parsed elements
    inline list<InetAddress> getParsedIPs() { return this->parsedIPs; }
    inline list<NetworkAddress> getParsedIPBlocks() { return this->parsedIPBlocks; }
    
    /*
     * Method to obtain (re-ordered) target IPs for scanning. It should be noted that 
     * the fact of obtaining the lists does not flush the content of parsedIPs/parsedIPBlocks.
     */
    
    list<InetAddress> getTargets();
    
    // Boolean method telling if the LAN of the VP is encompassed by the targets
    bool targetsEncompassLAN();

private:
    
    // Pointer to the environment (gives output stream and some useful parameters)
    ExploreNETEnvironment *env;

    // Private fields
    list<InetAddress> parsedIPs;
    list<NetworkAddress> parsedIPBlocks;
    
    // Private methods
    void parse(string input, char separator);
    list<InetAddress> reorder(list<InetAddress> toReorder);
};

#endif /* TARGETPARSER_H_ */
