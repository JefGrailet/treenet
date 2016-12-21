/*
 * SubnetParser.h
 *
 *  Created on: Jan 6, 2016
 *      Author: jefgrailet
 *
 * This class is dedicated to parsing the subnet dump(s) which are fed to TreeNET Reader at 
 * launch. Initially, the whole code was found in Main.cpp, but its size and the need to handle 
 * IP dictionnary parsing separately led to the creation of this class, for a more readable code. 
 * The code is otherwise very straightforward.
 *
 * The input file content, gathered in a single string, is parsed into separate subnets which are 
 * inserted in the set if they are compatible (i.e. not redundant with the subnets that were 
 * already inserted). For large dataset, an option allows user to skip the compatibility check to 
 * parse the input faster (if (s)he is sure there is no overlap at all).
 *
 * In case of incorrect formatting, error messages will be displayed in the console output.
 */
 
#ifndef SUBNETPARSER_H_
#define SUBNETPARSER_H_

#include <string>
using std::string;

#include "../TreeNETEnvironment.h"
#include "../structure/SubnetSite.h"

class SubnetParser
{
public:

    // Constructor, destructor
    SubnetParser(TreeNETEnvironment *env);
    ~SubnetParser();
    
    /*
     * Parsing method, storing the subnets extracted from an input file provided as a string into 
     * a given destination SubnetSiteSet. Originally, the method would store directly in the set 
     * to which env keeps a pointer, but this is not well suited for the merging mode where 
     * subnets from distinct files should be stored in separate subnet sets. The additional 
     * boolean parameter can be set to true in order to store the IPs found in the subnet dump in 
     * the IP dictionnary. This should be done if there is no IP dictionnary dump.
     */
    
    void parse(SubnetSiteSet *dest, string inputFileContent, bool storeInIPDict);

private:
    
    // Pointer to the environment variable
    TreeNETEnvironment *env;

};

#endif /* SUBNETPARSER_H_ */
