/*
 * Crow.h
 *
 *  Created on: Sept 13, 2016
 *      Author: grailet
 *
 * This more complex climber classes visits network trees to analyze their neighborhoods in depth, 
 * for example by performing the actual alias resolution and checking that the neighborhood has 
 * complete linkage or not. It is the equivalent of the old "internals()" method in NetworkTree 
 * class (TreeNET v2.3).
 */

#ifndef CROW_H_
#define CROW_H_

#include "../../aliasresolution/AliasResolver.h"
#include "Climber.h"

class Crow : public Climber
{
public:

    // Constructor, destructor
    Crow(TreeNETEnvironment *env);
    ~Crow(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual
    
    // Method to flush the obtained aliases into an output text file
    void outputAliases(string filename);

protected:

    // Alias resolution tool and list of produced aliases
    AliasResolver *ar;
    list<Router> aliases;
    
    // Field to maintain Soil while travelling (useful for the getSubnetContaining() method)
    Soil *soilRef;

    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter works just like 
     * in Robin class; its goal is to have a default TTL for un-probed IP during alias resolution.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
};

#endif /* CROW_H_ */
