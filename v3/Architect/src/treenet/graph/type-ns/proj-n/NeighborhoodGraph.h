/*
 * NeighborhoodGraph.h
 *
 *  Created on: Dec 1, 2016
 *      Author: jefgrailet
 *
 * This child of SimpleGraph models a graph that only consists of neighborhoods which are 
 * connected to each other if and only if one is the parent of the other in the corresponding 
 * network tree. Its construction is achieved by projecting a neighborhood - subnet bipartite 
 * graph on the neighborhoods.
 */

#ifndef NEIGHBORHOODGRAPH_H_
#define NEIGHBORHOODGRAPH_H_

#include <string>
using std::string;
#include <map>
using std::map;
using std::pair;

#include "../../SimpleGraph.h"
#include "../NSGraph.h"

class NeighborhoodGraph : public SimpleGraph
{
public:

    NeighborhoodGraph(NSGraph *toProject);
    ~NeighborhoodGraph();

protected:

    /*
     * Map and associated method to get the copy of some neighborhood while creating new edges, as 
     * all subnets (as vertices) are copy-constructed while projecting. This is useful for 
     * computing statistics at the end, because re-using the same objects in the projection will 
     * falsify metrics like the degree of the vertices (max., min., average, etc.).
     */
    
    map<string, Neighborhood*> copiedNeighborhoods;
    Neighborhood* getEquivalent(Neighborhood *original); // NULL if not found

};

#endif /* NEIGHBORHOODGRAPH_H_ */
