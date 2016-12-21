/*
 * Robin.h
 *
 *  Created on: Sept 13, 2016
 *      Author: jefgrailet
 *
 * This simple climber visits the tree to print out its structure in console, with one subnet or 
 * neighborhood per line along the associated depth.
 */

#ifndef ROBIN_H_
#define ROBIN_H_

#include "Climber.h"

class Robin : public Climber
{
public:

    // Constructor, destructor
    Robin(TreeNETEnvironment *env);
    ~Robin(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:

    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter is incremented 
     * each time we go deeper to be able to annotate each outputted line with the corresponding 
     * depth.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
};

#endif /* ROBIN_H_ */
