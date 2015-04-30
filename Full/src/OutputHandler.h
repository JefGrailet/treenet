/*
 * OutputHandler.h
 *
 *  Created on: Oct 12, 2014
 *      Author: grailet
 *
 * Output handler is a class solely dedicated to outputting topology discovery results, rather
 * than having print out methods scattered among several classes. It was made as a class so that
 * an OutputHandler object can keep some information (like a pointer to the output stream itself) 
 * about the print out method or policy, rather than having additionnal parameters for each print
 * out method/function.
 */
 
#ifndef OUTPUTHANDLER_H_
#define OUTPUTHANDLER_H_

#include "./toolbase/structure/SubnetSiteNode.h"
#include "./toolbase/structure/SubnetSite.h"

class OutputHandler
{
public:
    OutputHandler(ostream *out, bool resolvehostNames, bool showAlternatives);
    ~OutputHandler();
    
    // Setters
    inline void paramResolveHostNames(bool resolveHostNames) { this->resolveHostNames = resolveHostNames; }
    inline void paramShowAlternatives(bool showAlternatives) { this->showAlternatives = showAlternatives; }
    
    // Print out methods; their parameters depend on the type of message
    void usage(string programName);
    void printHeaderLines();
    void printSite(SubnetSite *site);

private:

    // Private fields
    ostream *out;
    bool resolveHostNames;
    bool showAlternatives;
    
    // Private methods (displaySite() decomposed in two methods)
    void printDiscoveredSite(SubnetSite *site);
    void printAlternativeSite(SubnetSite *site);
};

#endif /* OUTPUTHANDLER_H_ */

