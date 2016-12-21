/*
 * RSGraph.h
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * This child of BipartiteGraph models a bipartite graphs where the parties are, respectively, the 
 * inferred routers and the inferred subnets which connect them (when missing, the "links" are 
 * replaced by some imaginary component) found in a network tree. The class has been named 
 * "RSGraph" ("RS" standing for "Router - Subnet") for convenience. It constitutes one of the 
 * bipartite graphs of an ERS double bipartite graph (ERS = "Ethernet - Router - Subnet").
 */

#ifndef RSGRAPH_H_
#define RSGRAPH_H_

#include <map>
using std::map;
using std::pair;

#include "../BipartiteGraph.h"
#include "../L3Device.h"
#include "../Subnet.h"
#include "../../tree/NetworkTreeNode.h"

class RSGraph : public BipartiteGraph
{
public:

    RSGraph();
    ~RSGraph();
    
    // Methods to create edges.
    void createEdge(Router *router, SubnetSite *subnet);
    void createEdge(NetworkTreeNode *neighborhood, SubnetSite *subnet); // Will create imaginary router

    // Methods to create a virtual path between two (imaginary) routers via an imaginary subnet
    void createArtificialPath(Router *r1, Router *r2);
    void createArtificialPath(Router *r, NetworkTreeNode *n);
    void createArtificialPath(NetworkTreeNode *n1, NetworkTreeNode *n2);
    
    // Method to get a neighborhood matching an imaginary router (see copyRouters() in ERGraph)
    NetworkTreeNode *getNeighborhoodMatching(L3Device *imaginary);

protected:

    // Maps to keep track of elements already present in the graph
    map<NetworkTreeNode*, L3Device*> imaginaryRouters;
    map<Router*, L3Device*> routers;
    map<SubnetSite*, Subnet*> subnets;
    
    // Reverse map of imaginary routers (needed for when routers are copied in another graph)
    map<L3Device*, NetworkTreeNode*> neighborhoods;
    
    /*
     * Methods to insert a new router or subnet in the graph (creation of the vertices). 
     * Return the created element.
     */
    
    L3Device* insert(NetworkTreeNode *neighborhood);
    L3Device* insert(Router *router);
    Subnet* insert(SubnetSite *subnet);
    
    // Methods to retrieve a specific element if already in the graph (return NULL if absent)
    L3Device* lookUp(NetworkTreeNode *neighborhood);
    L3Device* lookUp(Router *router);
    Subnet* lookUp(SubnetSite *subnet);
    
    // Method to create artificial path between two L3Device objects (used by create...() methods)
    void connectL3Devices(L3Device *l3_1, L3Device *l3_2);
    
};

#endif /* RSGRAPH_H_ */
