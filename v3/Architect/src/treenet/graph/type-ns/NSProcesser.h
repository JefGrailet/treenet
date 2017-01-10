/*
 * NSProcesser.h
 *
 *  Created on: Nov 29, 2016
 *      Author: jefgrailet
 *
 * This class is made to turn a network tree into a bipartite graph made of neighborhoods and 
 * subnets and output the result in a new text file. The "NS" stands for "Neighborhood - Subnet".
 */

#ifndef NSPROCESSER_H_
#define NSPROCESSER_H_

#include "../../TreeNETEnvironment.h"
#include "../../tree/Soil.h"
#include "NSGraph.h"

class NSProcesser
{
public:

    NSProcesser(TreeNETEnvironment *env);
    ~NSProcesser();
    
    void process(Soil *fromSoil);
    void output(string filename);
    
    /*
     * check() returns a percentage of vertices being visited when travelling the graph starting 
     * from N1. Ideally, it should return 100% of covered vertices (single connected component). 
     * Otherwise, this should help calling code to advertise the user that there are several 
     * connected components (with the main one having a certain coverage). check() should be 
     * called after process(), and before output().
     */
    
    double check();
    
    void outputSubnetProjection(string filename);
    void outputNeighborhoodProjection(string filename);

protected:

    // Reference to environment (can be useful to get parameters/output stream)
    TreeNETEnvironment *env;

    // Reference to the tree being transformed (during a call to process)
    Soil *soilRef;
    
    /*
     * Will contain the result after a call to process(). It should be emptied after a call to 
     * output().
     */
    
    NSGraph *result;
    
    /*
     * Arrays used for checking the graph is made of a single connected component (via the check() 
     * method). These fields are useful for the visit() private method (see below) and are only 
     * set for it.
     */
    
    unsigned int nNeighborhoods, nSubnets;
    bool *checkArrN, *checkArrS;

    /*
     * Method to travel through the main trunk until we meet a node with multiple children. 
     * Turning that trunk into a bipartite representation is not very interesting. The method 
     * will immediately call processRecursive() (see below) afterwards.
     */
    
    void skipTrunkAndStart(NetworkTree *tree);

    // Method to recursively process the tree into a bipartite representation, node by node.
    void processRecursive(NetworkTreeNode *cur);
    
    // Method to visit a node and visits its neighbors except if already visited.
    void visit(Vertice *node);

};

#endif /* NSPROCESSER_H_ */
