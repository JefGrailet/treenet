/*
 * RouteInterface.h
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * A simple class to represent a single interface in a route (not to mix up with RouterInterface, 
 * which represents an interface on a router obtained through alias resolution). Initially, routes 
 * would only consist of an array of InetAddress objects, but for the needs of TreeNET v3.0 (and 
 * onwards), it is necessary to be able to give a "class" to an interface on the route, because a 
 * 0.0.0.0 InetAddress object can mean several things in v3.0 ("hole" during measurements, not 
 * measured because not useful, etc.).
 */

#ifndef ROUTEINTERFACE_H_
#define ROUTEINTERFACE_H_

#include "../../common/inet/InetAddress.h"

class RouteInterface
{
public:

    // Possible methods being used when this IP is associated to a router
    enum InterfaceStates
    {
        NOT_MEASURED, // Not measured yet
        MISSING, // Tried to get it via traceroute, without success
        REPAIRED, // Repaired via TreeNET v2.0+ tree construction method
        VIA_TRACEROUTE, // Obtained through traceroute
        PREDICTED // Predicted, i.e., after rewriting the route during a grafting process
    };

    RouteInterface(); // Creates a "NOT_MEASURED" interface
    RouteInterface(InetAddress ip);
    ~RouteInterface();
    
    void update(InetAddress ip); // For when the interface is initially "NOT_MEASURED"
    void repair(InetAddress ip); // Always sets state to "REPAIRED"
    
    // Overriden equality operator
    RouteInterface &operator=(const RouteInterface &other);
    
    InetAddress ip;
    unsigned short state;
    
};

#endif /* ROUTEINTERFACE_H_ */
