/*
 * IPDictionnaryParser.h
 *
 *  Created on: Jan 6, 2016
 *      Author: grailet
 *
 * Just like its name implies, this class is made to parse the dump of an IP dictionnary provided 
 * with a subnet dump. Unlike in its v1.0, where alias resolution hints were provided in the 
 * subnet dump, TreeNET now separates the subnets from an "IP dictionnary" which lists every 
 * responsive IP found in a measured domain, these IPs being almost all listed in subnets. The 
 * main advantage of this solution is that alias resolution hints are only written one time in a 
 * dump, and moreover, their amount can vary (since the user can now choose the amount of IP-IDs 
 * (s)he will collect for each IP that is a candidate for alias resolution).
 *
 * The main structure and usage of this class is identical to that of SubnetParser.
 */
 
#ifndef IPDICTIONNARYPARSER_H_
#define IPDICTIONNARYPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "TreeNETEnvironment.h"

class IPDictionnaryParser
{
public:

    // Constructor, destructor
    IPDictionnaryParser(TreeNETEnvironment *env);
    ~IPDictionnaryParser();
    
    // Parsing method (parsed IPs are directly put into the dictionnary of env)
    void parse(string inputFileContent);

private:
    
    // Pointer to the environment variable
    TreeNETEnvironment *env;
    
    // Private method to "explode" a string (i.e., split it into chunks at a given delimiter)
    static list<string> explode(string input, char delimiter);

};

#endif /* IPDICTIONNARYPARSER_H_ */
