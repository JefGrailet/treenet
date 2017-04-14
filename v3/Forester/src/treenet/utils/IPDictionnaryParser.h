/*
 * IPDictionnaryParser.h
 *
 *  Created on: Jan 6, 2016
 *      Author: jefgrailet
 *
 * Just like its name implies, this class is made to parse the dump of an IP dictionnary provided 
 * with a subnet dump. Unlike in its v1.0, where alias resolution hints were provided in the 
 * subnet dump, TreeNET now separates the subnets from an "IP dictionnary" which lists every 
 * responsive IP found in a measured domain, these IPs being almost all listed in subnets. The 
 * main advantage of this solution is that alias resolution hints are only written one time in a 
 * dump, and moreover, their amount can vary (since the user can now choose the amount of IP-IDs 
 * (s)he will collect for each IP that is a candidate for alias resolution).
 *
 * The main structure and usage of this class is identical to that of SubnetParser. As of March 
 * 2017, the parse method now returns a boolean value because it is also responsible for opening 
 * the file containing the data to parse (this minimizes code in Main.cpp). It also only displays 
 * error message if the display mode of TreeNET is set to "slightly verbose".
 *
 * Remark: there could have been a superclass for both SubnetParser and IPDictionnaryParser, 
 * however there was no strong similarity to motivate this additionnal class.
 */
 
#ifndef IPDICTIONNARYPARSER_H_
#define IPDICTIONNARYPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "../TreeNETEnvironment.h"

class IPDictionnaryParser
{
public:

    // Constructor, destructor
    IPDictionnaryParser(TreeNETEnvironment *env);
    ~IPDictionnaryParser();
    
    // Parsing method (returns true if a file was opened and parsed)
    bool parse(string inputFileName);

private:
    
    // Pointer to the environment variable
    TreeNETEnvironment *env;
    
    // Private method to "explode" a string (i.e., split it into chunks at a given delimiter)
    static list<string> explode(string input, char delimiter);
    
    // Fields to count, during parsing, correctly parsed IPs and the bad ones, along the total
    unsigned int parsedLines, badLines, unusableLines, totalLines;

};

#endif /* IPDICTIONNARYPARSER_H_ */
