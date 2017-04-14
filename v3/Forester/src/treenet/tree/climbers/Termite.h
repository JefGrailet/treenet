/*
 * Termite.h
 *
 *  Created on: Feb 22, 2017
 *      Author: jefgrailet
 *
 * Termite class visits a tree, neighborhood by neighborhood, and evaluates the probability that 
 * some routers are connected (or not) to L2 equipment. It then displays a probability per router 
 * and then evaluates the probability that the whole neighborhood contains L2 equipment.
 */

#ifndef TERMITE_H_
#define TERMITE_H_

#include "Climber.h"

class Termite : public Climber
{
public:

    // Constructor, destructor
    Termite(TreeNETEnvironment *env);
    ~Termite(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:

    // Method to evaluate probability of having L2 in a (sub-)neighborhood (as a list of routers)
    void evaluate(list<Router*> routers);

    // Method to recursively "climb" the tree, node by node.
    void climbRecursive(NetworkTreeNode *cur);
};

#endif /* TERMITE_H_ */
