/*
 * Router.h
 *
 *  Created on: Nov 5, 2015
 *      Author: grailet
 *
 * A class to represent a router (or rather, an aliases list) which will be used to assess the 
 * alias resolution techniques of TreeNET v2.0. It is NOT equivalent to the Router class from the 
 * full version of TreeNET (this one has been renamed as InferredRouter), as the goal of this 
 * new class is to maintain the expected result such that it can be automatically compared with 
 * what TreeNET obtains.
 *
 * To this end, this class maintains 3 lists:
 * -one with the expected IPs (as InetAddress objects)
 * -one with the unresponsive IPs
 * -one of InferredRouter objects with the alias resolution results of TreeNET. Ideally, it should 
 *  contain a single object with the same interfaces as in the first list, minus the unresponsive 
 *  IPs (if any).
 *
 * Somehow, this class also replaces the "Neighborhoods" in TreeNET, as all IPs from the initial 
 * data set are assumed to belong to the same Neighborhood (since they are from the same router).
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "./InferredRouter.h"

class Router
{
public:

    Router();
    ~Router();
    
    // Accessers to the lists
    inline list<InetAddress> *getExpectedIPs() { return &expected; }
    inline list<InetAddress> *getUnresponsiveIPs() { return &unresponsive; }
    inline list<InferredRouter> *getInferredRouters() { return &inferredRouters; }

    // Method to add new elements to each list
    void addExpectedIP(InetAddress ia);
    void addUnresponsiveIP(InetAddress ia);
    void addInferredRouter(InferredRouter ir);
    
    // Method to get the amount of elements from each list
    unsigned short getNbExpectedIPs();
    unsigned short getNbUnresponsiveIPs();
    unsigned short getNbInferredRouters();
    
    // Method to know if this IP list could be analyzed or not
    bool canBeInferred();
    
    // N.B.: next methods should be called only after alias resolution.
    
    // Method to tell if the inferred results match the expected ones at 100%
    bool correctlyInferred();
    
    // Method to get a success ratio (in case it is not a success at 100%)
    double getSuccessRatio();
    
    // Finally, a method to output the object
    string toString();
    
private:

    // The different lists as mentioned above
    list<InetAddress> expected, unresponsive;
    list<InferredRouter> inferredRouters;
};

#endif /* ROUTER_H_ */
