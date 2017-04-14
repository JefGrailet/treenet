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
 * In case of incorrect formatting, error messages will be written to the output stream if the 
 * display mode is set to "slightly verbose" (March 2017).
 *
 * In March 2017, the class has been rehashed to move away some code of the Main.cpp file and put 
 * elsewhere the responsibility of filling the IP dictionnary with interfaces mentioned in the 
 * subnets.
 *
 * Remark: there could have been a superclass for both SubnetParser and IPDictionnaryParser, 
 * however there was no strong similarity to motivate this additionnal class.
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
    
    // Parsing methods (returns true if a file was indeed parsed)
    bool parse(string inputFileName);
    bool parse(string inputFileName, SubnetSiteSet *dest);

private:
    
    // Pointer to the environment variable
    TreeNETEnvironment *env;
    
    // Fields to count, during parsing, correctly parsed subnets along merging and bad parsings
    unsigned int parsedSubnets, credibleSubnets, mergedSubnets, duplicateSubnets, badSubnets;
    
    /*
     * "True" parsing method, storing the subnets extracted from an input file provided as a 
     * string into a given destination SubnetSiteSet. Originally, the method would store directly 
     * in the set to which env keeps a pointer, but this is not well suited for the merging mode 
     * of Forester where subnets from distinct files should be stored in separate subnet sets.
     */
    
    void parse(SubnetSiteSet *dest, string inputFileContent);

};

#endif /* SUBNETPARSER_H_ */
