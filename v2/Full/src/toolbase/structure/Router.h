/*
 * Router.h
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * A simple class to represent an inferred router (during alias resolution). It only consists in 
 * a list of interfaces (as RouterInterface objects).
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "RouterInterface.h"

class Router
{
public:

    Router();
    ~Router();
    
    // Accessor to the list
    inline list<RouterInterface> *getInterfacesList() { return &interfaces; }

    // Method to add a new interface to this router.
    void addInterface(InetAddress interface, unsigned short aliasMethod);
    
    // Method to get the amount of interfaces of this router.
    unsigned short getNbInterfaces();
    
private:

    // Interfaces are stored with a list
    list<RouterInterface> interfaces;
};

#endif /* ROUTER_H_ */
