/*
 * Cuckoo.h
 *
 *  Created on: Sept 13, 2016
 *      Author: grailet
 *
 * Cuckoo class is responsible for visiting the tree and collecting the alias resolution hints for 
 * each neighborhood - therefore modifying the tree (or rather data structures of TreeNET), hence 
 * the name of the class. It is equivalent to the former method "collectAliasResolutionHints()" 
 * found in NetworkTree class in TreeNET v2.0+.
 */

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include "../../aliasresolution/AliasHintCollector.h"
#include "Climber.h"

class Cuckoo : public Climber
{
public:

    // Constructor, destructor
    Cuckoo(TreeNETEnvironment *env);
    ~Cuckoo(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:

    AliasHintCollector *ahc;

    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter works just like 
     * in Robin class; its goal is to have a default TTL for previously un-probed IP during alias 
     * hint collection.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
};

#endif /* CUCKOO_H_ */
