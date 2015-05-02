/*
 * BipartiteSubnet.h
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it represents a subnet from the point of view of a 
 * bipartite graph.
 * 
 * Since it is not possible to infer all the subnets so that one can have a subnet to represent 
 * all links between routers, this class allows the definition of "imaginary" subnets to connect 
 * pair of routers which are actually connected (because they were traversed with the same 
 * traceroute) but for which we could not find the subnets between these routers.
 *
 * This class also stores a convenient notation for subnets: S[ID]. The first subnet to be used
 * in the bipartite will be labelled S1, the second S2... etc. An output file will describe this 
 * notation (i.e. which label corresponds to which subnet).
 */

#ifndef BIPARTITESUBNET_H_
#define BIPARTITESUBNET_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "LinkRouterSubnet.h"

// Forward declarations
class SubnetSite;
class LinkRouterSubnet;

class BipartiteSubnet
{
public:

    // Types of node
    enum SubnetType
    {
        T_INFERRED,
        T_IMAGINARY
    };

    BipartiteSubnet(string label); // For imaginary subnet
    BipartiteSubnet(string label, SubnetSite *subnet); // For inferred subnet
    ~BipartiteSubnet();

    // Accessors
    inline string getLabel() const { return label; }
    inline unsigned short getType() const { return type; }
    inline SubnetSite *getAssociatedSubnet() { return associatedSubnet; }
    inline list<LinkRouterSubnet*> *getLinks() { return &links; }
    
    // Methods to handle the links
    bool isConnectedTo(string routerLabel);
    void removeConnectionTo(string routerLabel);
    inline void addLink(LinkRouterSubnet *link) { links.push_back(link); }
    
private:

    // Label, type, associated subnet (if any)
    string label;
    unsigned short type;
    SubnetSite *associatedSubnet;
    
    // List of links connected to this element
    list<LinkRouterSubnet*> links;

};

#endif /* BIPARTITESUBNET_H_ */
