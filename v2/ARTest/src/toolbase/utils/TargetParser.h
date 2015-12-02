/*
 * TargetParser.h
 *
 *  Created on: Nov 5, 2015
 *      Author: grailet
 *
 * This class is dedicated to parsing the target IPs which are fed to TreeNET at launch. This 
 * class is however very different from the one found in the full version of TreeNET, mostly 
 * because there is only prescanning, no expansion, a different kind of input file with a more 
 * elaborated output (list of Router objects; not to mix with the Router class from the full 
 * TreeNET).
 */
 
#ifndef TARGETPARSER_H_
#define TARGETPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "../TreeNETEnvironment.h"
#include "../../common/inet/NetworkAddress.h"

class TargetParser
{
public:

    // Constructor, destructor
    TargetParser(TreeNETEnvironment *env);
    ~TargetParser();
    
    // Parsing method
    void parseInputFile(string inputFileContent);
    
    // Accesser to parsed IPs
    inline list<InetAddress> getParsedIPs() { return this->parsedIPs; }
    
    /*
     * Method to obtain (re-ordered) target IPs for scanning/prescanning. It should be noted that 
     * the fact of obtaining the list does not flush the content of parsedIPs.
     */
    
    list<InetAddress> getTargets();
    
    // Boolean method telling if the LAN of the VP contains some of the targets
    bool targetsInLAN();

private:
    
    // Pointer to the environment (gives output stream and some useful parameters)
    TreeNETEnvironment *env;

    // Private field
    list<InetAddress> parsedIPs;
    
    // Private method (identical to what can be found in the Full TreeNET)
    list<InetAddress> reorder(list<InetAddress> toReorder);
};

#endif /* TARGETPARSER_H_ */
