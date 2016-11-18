/*
 * Cat.h
 *
 *  Created on: Nov 3, 2016
 *      Author: jefgrailet
 *
 * This climber class visits network trees to analyze in depth each neighborhood. This consists 
 * by printing the list of subnets, fingerprints and obtained aliases, with the linkage that is 
 * observed between distinct neighborhoods. It is the equivalent of the old neighborhoods() method 
 * in previous versions of TreeNET (Reader).
 *
 * Initially, the same task was also carried out by Crow (which now just does the alias 
 * resolution). Both operations were separated to ease the re-usage of the code between the 
 * different versions of TreeNET (in particular, Arborist and Forester).
 */

#ifndef CAT_H_
#define CAT_H_

#include "../../aliasresolution/Fingerprint.h"
#include "Climber.h"

class Cat : public Climber
{
public:

    // Constructor, destructor
    Cat(TreeNETEnvironment *env);
    ~Cat(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:

    // Field to maintain Soil while travelling (useful for the getSubnetContaining() method)
    Soil *soilRef;

    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter works just like 
     * in Robin class; its goal is to have a default TTL for un-probed IP during alias resolution.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
};

#endif /* CAT_H_ */
