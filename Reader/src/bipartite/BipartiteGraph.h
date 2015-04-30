/*
 * BipartiteGraph.h
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * This class models a bipartite graph made of routers connected to subnets. The underlying idea 
 * is that there is a subnet between each point-to-point link between routers.
 *
 * To implement this, the class is made of several components: two classes modeling respectively 
 * the routers and the subnets and a third class modeling the links between them. These components 
 * can be used for "real" components (i.e. inferred during the topology discovery) as well as 
 * "imaginary" components which aimed at reconnecting components for which no router/subnet could 
 * be discovered.
 *
 * However, since it is not always possible to connect together all components of the topology, a
 * second bipartite graph (L2/Ethernet switches - L3/routers) is necessary to reconnect routers 
 * belonging to the same neighborhood with the help of an (inferred) switch. To implement this, 
 * 2 additionnal classes BipartiteSwitch and LinkSwitchRouter were designed as well. This makes 
 * the graph technically a double bipartite, while the focus is on the router/subnet side.
 */

#ifndef BIPARTITEGRAPH_H_
#define BIPARTITEGRAPH_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "../structure/Router.h"
#include "../structure/SubnetSite.h"
#include "BipartiteSwitch.h"
#include "BipartiteRouter.h"
#include "BipartiteSubnet.h"
#include "LinkRouterSubnet.h"
#include "LinkSwitchRouter.h"

class BipartiteGraph
{
public:

	BipartiteGraph();
	~BipartiteGraph();

	// Accessors
	inline list<BipartiteSwitch*> *getSwitches() { return &switches; }
	inline list<BipartiteRouter*> *getRouters() { return &routers; }
	inline list<BipartiteSubnet*> *getSubnets() { return &subnets; }
	inline list<LinkSwitchRouter*> *getLinksSR() { return &linksSR; }
	inline list<LinkRouterSubnet*> *getLinksRS() { return &linksRS; }
	
	/*
	 * Methods to add a new component to the graph. When the component (i.e. an instance of 
	 * BipartiteRouter or BipartiteSubnet) is created, a pointer to it is kept by the 
	 * corresponding Router/SubnetSite instances so that a link between them can be easily
	 * created afterwards (see below).
	 *
	 * Additionnaly, this pointer can be used to quickly now if a router or subnet exists in the 
	 * graph or not.
	 */
    
	void addRouter(Router *r);
	void addSubnet(SubnetSite *ss);
	
	/*
	 * Methods to create and insert imaginary components. This time, the component is returned, 
	 * as it cannot be retrieved from corresponding Router/Subnet objects and from nothing in the 
	 * case of a switch.
	 */
	
	BipartiteSwitch *createSwitch();
	BipartiteRouter *createImaginaryRouter();
	BipartiteSubnet *createImaginarySubnet();
	
	// Method to create new Switch-Router or Router-Subnet links.
	void createLinkSR(BipartiteSwitch *bipSwitch, BipartiteRouter *bipRouter);
	void createLinkRS(BipartiteRouter *bipRouter, BipartiteSubnet *bipSubnet);
	
	// Method to conveniently create a new link between a measured router and a measured subnet.
	void createLink(Router *r, SubnetSite *ss);
	
	// Methods to remove (any) Switch-Router or Router-Subnet link on the basis of labels.
	void removeLinkSR(string switchLabel, string routerLabel);
	void removeLinkRS(string routerLabel, string subnetLabel);
	
	/*
	 * toString() methods; they respectively return list presenting each component with their 
	 * notation (e.g. S1 - v.w.x.y/z, one per line) and a list of links existing between them 
	 * (the links using the notation, e.g. "R1 - S3").
	 *
	 * N.B.: there is no "switchesToString()" method as the notation appearing in linksSR is 
	 * enough (switches correspond to nothing in the measures, unlike routers and subnets).
	 */
	
	string routersToString();
	string subnetsToString();
	string linksSRToString();
	string linksRSToString();
	
private:

    // List of components
    list<BipartiteSwitch*> switches;
    list<BipartiteRouter*> routers;
    list<BipartiteSubnet*> subnets;
    list<LinkSwitchRouter*> linksSR;
    list<LinkRouterSubnet*> linksRS;
    
    // Counters for the notation of each element
    unsigned int switchCounter;
    unsigned int routerCounter;
    unsigned int subnetCounter;

};

#endif /* BIPARTITEGRAPH_H_ */

