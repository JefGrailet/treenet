/*
 * NSGraph.h
 *
 *  Created on: Nov 29, 2016
 *      Author: jefgrailet
 *
 * This child of BipartiteGraph models a bipartite graphs where the parties are, respectively, the 
 * neighborhoods inferred by a network tree and the subnets which connect them (when missing, the 
 * "links" are replaced by some imaginary subnet). The class has been named "NSGraph" ("NS" 
 * standing for "Neighborhood - Subnet") for convenience.
 */

#ifndef NSGRAPH_H_
#define NSGRAPH_H_

#include <map>
using std::map;
using std::pair;

#include "../BipartiteGraph.h"
#include "../Neighborhood.h"
#include "../Subnet.h"

class NSGraph : public BipartiteGraph
{
public:

    NSGraph();
    ~NSGraph();
    
    /*
     * Methods to create edges. createEdge() creates an actual Neighborhood - Subnet link, while 
     * createArtificialPath creates 2 consecutive edges which are both incident to an imaginary 
     * subnet in order to create a path between two neighborhoods.
     */
    
    void createEdge(NetworkTreeNode *neighborhood, SubnetSite *subnet);
    void createArtificialPath(NetworkTreeNode *neighborhood1, NetworkTreeNode *neighborhood2);

protected:

    // Maps to keep track of elements already present in the graph
    map<NetworkTreeNode*, Neighborhood*> neighborhoods;
    map<SubnetSite*, Subnet*> subnets;
    
    /*
     * Methods to insert a new neighborhood or subnet in the graph (creation of the vertices). 
     * Return the created element.
     */
    
    Neighborhood* insert(NetworkTreeNode *neighborhood);
    Subnet* insert(SubnetSite *subnet);
    
    // Methods to check if a neighborhood/subnet already is in the graph (return NULL if absent)
    Neighborhood* lookUp(NetworkTreeNode *neighborhood);
    Subnet* lookUp(SubnetSite *subnet);
    
};

#endif /* NSGRAPH_H_ */
