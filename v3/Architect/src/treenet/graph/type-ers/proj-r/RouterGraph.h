/*
 * RouterGraph.h
 *
 *  Created on: Dec 14, 2016
 *      Author: jefgrailet
 *
 * This child of SimpleGraph models a graph that only consists of routers which are connected to 
 * each other if and only if they are separated by a single subnet or ethernet switch. Its 
 * construction is achieved by projecting an either switch - router - subnet double bipartite 
 * graph on the routers.
 */

#ifndef ROUTERGRAPH_H_
#define ROUTERGRAPH_H_

#include <string>
using std::string;
#include <map>
using std::map;
using std::pair;

#include "../../SimpleGraph.h"
#include "../ERGraph.h"
#include "../RSGraph.h"

class RouterGraph : public SimpleGraph
{
public:

    RouterGraph(ERGraph *toProject1, RSGraph *toProject2);
    ~RouterGraph();

protected:

    /*
     * Map and associated method to get the copy of some router (as L3Device) while creating new 
     * edges, as all routers (as vertices) are copy-constructed while projecting. This is useful 
     * for computing statistics at the end, because re-using the same objects in the projection 
     * will falsify metrics like the degree of the vertices (max., min., average, etc.).
     */
    
    map<string, L3Device*> copiedRouters;
    L3Device* getEquivalent(L3Device *original); // NULL if not found

};

#endif /* ROUTERGRAPH_H_ */
