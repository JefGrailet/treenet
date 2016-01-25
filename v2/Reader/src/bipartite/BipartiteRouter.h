/*
 * BipartiteRouter.h
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it represents a router from the point of view of a 
 * bipartite graph.
 * 
 * Since it is not possible to infer exactly all routers, this class allows the definition of 
 * "imaginary" routers which connect subnets which are in the same neighborhood to the parent 
 * neighborhood when no inferred router is available. When the router exists, a pointer to the 
 * corresponding object (from src/structure/) is kept.
 *
 * This class also stores a convenient notation for routers: R[ID]. The first router to be used
 * in the bipartite will be labelled R1, the second R2... etc. An output file will describe this 
 * notation (i.e. which label corresponds to which router).
 */

#ifndef BIPARTITEROUTER_H_
#define BIPARTITEROUTER_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "LinkSwitchRouter.h"
#include "LinkRouterSubnet.h"

// Forward declarations
class Router;
class LinkRouterSubnet;
class LinkSwitchRouter;

class BipartiteRouter
{
public:

    // Types of node
    enum RouterType
    {
        T_INFERRED,
        T_IMAGINARY
    };

    BipartiteRouter(string label); // For imaginary router
    BipartiteRouter(string label, Router *router); // For inferred router
    ~BipartiteRouter();

    // Accessors
    inline string getLabel() const { return label; }
    inline unsigned short getType() const { return type; }
    inline Router *getAssociatedRouter() { return associatedRouter; }
    inline list<LinkSwitchRouter*> *getLinksSwitches() { return &linksSwitches; }
    inline list<LinkRouterSubnet*> *getLinksSubnets() { return &linksSubnets; }
    
    // Methods to handle the links
    bool isConnectedToSwitch(string switchLabel);
    void removeConnectionToSwitch(string switchLabel);
    inline void addLinkSwitch(LinkSwitchRouter *link) { linksSwitches.push_back(link); }
    
    bool isConnectedToSubnet(string subnetLabel);
    void removeConnectionToSubnet(string subnetLabel);
    inline void addLinkSubnet(LinkRouterSubnet *link) { linksSubnets.push_back(link); }
    
private:

    // Label, type, associated router (if any)
    string label;
    unsigned short type;
    Router *associatedRouter;
    
    // List of links connected to this element
    list<LinkSwitchRouter*> linksSwitches;
    list<LinkRouterSubnet*> linksSubnets;

};

#endif /* BIPARTITEROUTER_H_ */
