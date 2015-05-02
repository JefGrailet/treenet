/*
 * LinkRouterSubnet.h
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it represents a link between a router and a subnet 
 * in a bipartite graph.
 */

#ifndef LINKROUTERSUBNET_H_
#define LINKROUTERSUBNET_H_

#include <string>
using std::string;

#include "BipartiteRouter.h"
#include "BipartiteSubnet.h"

// Forward declarations
class BipartiteRouter;
class BipartiteSubnet;

class LinkRouterSubnet
{
public:

    LinkRouterSubnet(BipartiteRouter *router, BipartiteSubnet *subnet);
    ~LinkRouterSubnet();

    // Accessors
    inline BipartiteRouter *getRouter() { return router; }
    inline BipartiteSubnet *getSubnet() { return subnet; }
    
    // Boolean method returning true if the link is made of "real" (inferred) components
    bool isRealLink();
    
    // Returns a notation for this link (e.g. "R3 - S5")
    string toString();
    
private:

    // Associated router and subnet
    BipartiteRouter *router;
    BipartiteSubnet *subnet;

};

#endif /* LINKROUTERSUBNET_H_ */
