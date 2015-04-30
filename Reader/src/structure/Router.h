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
#include <string>
using std::string;

#include "SubnetSite.h"
#include "../common/inet/InetAddress.h"
#include "../bipartite/BipartiteRouter.h"

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
	
	// Method to check a given IP is an interface of this router.
	bool hasInterface(InetAddress interface);
	
	// Method to verify a given subnet is accessed through this router.
	bool givesAccessTo(SubnetSite *ss);
	
	// Converts the router into a string
	string toString();
	
	// Accessor/setter to the bipartite element
	inline bool hasBipEquivalent() { return this->bipRouter != NULL; }
	inline void setBipEquivalent(BipartiteRouter *bipRouter) { this->bipRouter = bipRouter; }
	inline BipartiteRouter *getBipEquivalent() { return this->bipRouter; }
	
private:

    // Interfaces are stored with a list
    list<InetAddress> interfacesList;
    
    // Corresponding bipartite element
    BipartiteRouter *bipRouter;
    
};

#endif /* ROUTER_H_ */

