/*
 * Sparrow.h
 *
 *  Created on: Aug 22, 2017
 *      Author: jefgrailet
 *
 * Sparrow class is responsible for visiting the tree and building the aggregates of IP interfaces 
 * which are likely to be aliases of each other. More precisely:
 * -for single-label nodes, it builds one and only one aggregate consisting of the label and the 
 *  contra-pivot nodes of the child subnets.
 * -for multi-label nodes, it first resolves potential aliases between the labels of the node and 
 *  turns the results into the bases of the aggregates.
 * It should be noted that this class does not print anything.
 */

#ifndef SPARROW_H_
#define SPARROW_H_

#include "Climber.h"

class Sparrow : public Climber
{
public:

    // Constructor, destructor
    Sparrow(TreeNETEnvironment *env);
    ~Sparrow(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:
    
    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter works just like 
     * in Robin class; its goal is to have a default TTL for previously un-probed IP during alias 
     * hint collection.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
};

#endif /* SPARROW_H_ */
