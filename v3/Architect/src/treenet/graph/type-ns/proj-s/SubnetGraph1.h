/*
 * SubnetGraph1.h
 *
 *  Created on: Dec 1, 2016
 *      Author: jefgrailet
 *
 * This child of SimpleGraph models a graph that only consists of subnets which are connected to 
 * each other if and only if they are separated by one router hop (one neighborhood in the network 
 * tree). Its construction is achieved by projecting a neighborhood - subnet bipartite graph on 
 * the subnets.
 *
 * Remark (Dec 15, 2016): the class has been renamed SubnetGraph1. Indeed, there is another 
 * SubnetGraph class (SubnetGraph2, found in src/treenet/graph/type-ers/proj-s/) which produces a 
 * subnet graph from an ERS graph.
 */

#ifndef SUBNETGRAPH1_H_
#define SUBNETGRAPH1_H_

#include <string>
using std::string;
#include <map>
using std::map;
using std::pair;

#include "../../SimpleGraph.h"
#include "../NSGraph.h"

class SubnetGraph1 : public SimpleGraph
{
public:

    SubnetGraph1(NSGraph *toProject);
    ~SubnetGraph1();

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
