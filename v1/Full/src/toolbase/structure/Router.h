/*
 * Router.h
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * A simple class to represent an inferred router (during alias resolution). It only consists in 
 * a list of interfaces (as simple InetAddress).
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"

class Router
{
public:

    Router();
    ~Router();
    
    // Accessor to the list
    inline list<InetAddress> *getInterfacesList() { return &interfacesList; }

    // Method to add a new interface to this router.
    void addInterface(InetAddress interface);
    
    // Method to get the amount of interfaces of this router.
    unsigned short getNbInterfaces();
    
private:

    // Interfaces are stored with a list
    list<InetAddress> interfacesList;
};

#endif /* ROUTER_H_ */
