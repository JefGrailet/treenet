/*
 * OutputHandler.h
 *
 *  Created on: Jan 6, 2016
 *      Author: grailet
 *
 * This class was made to separate the print out code from the Main.cpp file of TreeNET Reader.
 * Other than that, this class does not fill any other role and is very straigthforward.
 */
 
#ifndef OUTPUTHANDLER_H_
#define OUTPUTHANDLER_H_

#include <string>
using std::string;

#include "../structure/SubnetSite.h"

class OutputHandler
{
public:

    // Constructor, destructor
    OutputHandler(ostream *out);
    ~OutputHandler();
    
    // Displays usage
    void usage(string programName);
    
    // Displays subnets (headers + actual subnet subnet)
    void printHeaderLines();
    void printSubnetSite(SubnetSite *site);

private:
    
    // Pointer to the output stream
    ostream *out;

};

#endif /* OUTPUTHANDLER_H_ */
