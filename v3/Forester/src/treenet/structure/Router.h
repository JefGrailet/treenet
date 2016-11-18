/*
 * Router.h
 *
 *  Created on: Mar 1, 2015
 *      Author: jefgrailet
 *
 * A simple class to represent an inferred router (during alias resolution). It only consists in 
 * a list of interfaces (as RouterInterface objects).
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "./IPLookUpTable.h"
#include "./RouterInterface.h"

class Router
{
public:

    Router();
    ~Router();
    
    // Accessor to the list
    inline list<RouterInterface*> *getInterfacesList() { return &interfaces; }

    // Method to add a new interface to this router.
    void addInterface(InetAddress interface, unsigned short aliasMethod);
    
    // Method to get the amount of interfaces of this router.
    unsigned short getNbInterfaces();
    
    // Method to check a given IP is an interface of this router.
    bool hasInterface(InetAddress interface);
    
    /*
     * Method to get an interface fitting this criteria:
     * -aliased with the UDP-based method.
     * -it is listed as a "healthy" IP in the IP dictionnary.
     * Such an interface is a "merging pivot", i.e., the interface that will be used to merge an 
     * existing router solely obtained via UDP with interfaces aliased through the Ally method.
     * The corresponding entry in the dictionnary is returned. The said dictionnary must be 
     * provided as a parameter (because having a pointer to the environment variable in Router 
     * does not make a lot of sense).
     */
    
    IPTableEntry *getMergingPivot(IPLookUpTable *table);
    
    // Converts the Router object to an alias in string format
    string toString();
    
private:

    // Interfaces are stored with a list
    list<RouterInterface*> interfaces;
};

#endif /* ROUTER_H_ */
