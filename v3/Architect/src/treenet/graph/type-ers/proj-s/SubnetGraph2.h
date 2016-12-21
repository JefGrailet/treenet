/*
 * SubnetGraph2.h
 *
 *  Created on: Dec 19, 2016
 *      Author: jefgrailet
 *
 * This child of SimpleGraph models a graph that only consists of subnets which are connected to 
 * each other if and only if they are separated by one router hop (one neighborhood in the network 
 * tree). Its construction is achieved by projecting an ERS double bipartite graph on the subnets.
 */

#ifndef SUBNETGRAPH2_H_
#define SUBNETGRAPH2_H_

#include <string>
using std::string;
#include <map>
using std::map;
using std::pair;

#include "../../SimpleGraph.h"
#include "../ERGraph.h"
#include "../RSGraph.h"

class SubnetGraph2 : public SimpleGraph
{
public:

    SubnetGraph2(ERGraph *toProject1, RSGraph *toProject2);
    ~SubnetGraph2();

protected:

    /*
     * Map and associated method to get the copy of some subnet while creating new edges, as all 
     * subnets (as vertices) are copy-constructed while projecting. This is useful for computing 
     * statistics at the end, because re-using the same objects in the projection will falsify 
     * metrics like the degree of the vertices (max., min., average, etc.).
     */
    
    map<string, Subnet*> copiedSubnets;
    Subnet* getEquivalent(Subnet *original); // NULL if not found

};

#endif /* SUBNETGRAPH1_H_ */
