/*
 * BipartiteSwitch.h
 *
 *  Created on: Mar 6, 2015
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it represents an Ethernet switch (or more 
 * generally, a Layer-2 device) in the bipartite graph. It should be noted that this makes the 
 * the graph actually double bipartite: there is a bipartite between switches and routers and 
 * between routers and subnets.
 *
 * While the focus is on the bipartite routers/subnets, the bipartite switches/routers helps to 
 * get together the different components of the network. Indeed, routers inferred in a same 
 * neighborhood must be connected in some way, but within a neighborhood the only way to connect 
 * them is to use a Layer-2 device.
 *
 * It should be pointed out that this class is exclusively "imaginary", as Layer-2 devices are 
 * invisible to packets. However, it is very likely that such devices exist if the router 
 * inference is accurate.
 */

#ifndef BIPARTITESWITCH_H_
#define BIPARTITESWITCH_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "LinkSwitchRouter.h"

class LinkSwitchRouter; // Forward declaration

class BipartiteSwitch
{
public:

	BipartiteSwitch(string label);
	~BipartiteSwitch();

	// Accessors
	inline string getLabel() const { return label; }
	inline list<LinkSwitchRouter*> *getLinks() { return &links; }
	
	// Methods to handle the links
	bool isConnectedTo(string routerLabel);
	void removeConnectionTo(string routerLabel);
	inline void addLink(LinkSwitchRouter *link) { links.push_back(link); }
	
private:

    // Label
    string label;
    
    // List of links connected to this element
    list<LinkSwitchRouter*> links;

};

#endif /* BIPARTITESWITCH_H_ */

