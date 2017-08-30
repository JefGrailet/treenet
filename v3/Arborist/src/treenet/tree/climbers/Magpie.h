/*
 * Magpie.h
 *
 *  Created on: Aug 23, 2017
 *      Author: jefgrailet
 *
 * Magpie class is responsible for visiting the tree once to perform alias resolution hint 
 * collection on labels of multi-label nodes only. It is the first climber to be used in the 
 * improved alias resolution of TreeNET v3.3, where two additionnal steps ensure the multi-label 
 * nodes are accurately measured (from an alias point of view). Doing alias resolution only on 
 * labels of multi-label nodes before conducting the full alias resolution is meant to verify that 
 * such labels are alias of each other themselves. Indeed, it has already been observed with 
 * TreeNET that two interfaces appearing in routes at the same depth and eventually gathered in a 
 * multi-label nodes are actually on a same device.
 */

#ifndef MAGPIE_H_
#define MAGPIE_H_

#include "../../aliasresolution/AliasHintCollector.h"
#include "../../aliasresolution/AliasResolver.h"
#include "Climber.h"

class Magpie : public Climber
{
public:

    // Constructor, destructor
    Magpie(TreeNETEnvironment *env);
    ~Magpie(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual

protected:

    AliasHintCollector *ahc;
    AliasResolver *ar;

    /*
     * Method to recursively "climb" the tree, node by node. The depth parameter works just like 
     * in Robin class; its goal is to have a default TTL for previously un-probed IP during alias 
     * hint collection.
     */
    
    void climbRecursive(NetworkTreeNode *cur, unsigned short depth);
    
    /*
     * Private method to filter the interfaces of a "router" obtained by pre-aliasing. The goal is 
     * to record in the IP dictionnary only the pre-aliases obtained by UDP-based or IP-ID-based 
     * methods, because other approaches are more error-prone.
     *
     * @param Router* preAlias  The iniatial pre-alias
     */
    
    void filterPreAlias(Router *preAlias);
};

#endif /* MAGPIE_H_ */
