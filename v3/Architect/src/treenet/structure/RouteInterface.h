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
 * 0.0.0.0 InetAddress object can mean several things in v3.0 (rate-limited IP, "truly anonymous" 
 * interface, etc.).
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
        ANONYMOUS, // Tried to get it via traceroute, but resulted in a timeout at any time
        LIMITED, // Same, but got something when retrying later (because of rate-limitation or firewall)
        REPAIRED_1, // Repaired at first offline fix
        REPAIRED_2, // Repaired offline after re-covering a "limited" IP in another route
        VIA_TRACEROUTE, // Obtained through traceroute
        PREDICTED, // Predicted after using a grafting process
        STRETCHED, // This IP turned out to have been reachable with less hops
        CYCLE // This IP is a duplicate in the route
    };

    RouteInterface(); // Creates a "NOT_MEASURED" interface
    RouteInterface(InetAddress ip, bool timeout = false);
    ~RouteInterface();
    
    void update(InetAddress ip); // For when the interface is initially "NOT_MEASURED"
    void anonymize(); // Same but for when there is a timeout (= anonymous router)
    void repair(InetAddress ip); // Always sets state to "REPAIRED_1"
    void repairBis(InetAddress ip); // Always sets state to "REPAIRED_2"
    void deanonymize(InetAddress ip); // Always sets state to "LIMITED"
    void predict(InetAddress ip); // Always sets state to "PREDICTED"
    
    // Overriden equality operator
    RouteInterface &operator=(const RouteInterface &other);
    
    InetAddress ip;
    unsigned short state;
    
};

#endif /* ROUTEINTERFACE_H_ */
