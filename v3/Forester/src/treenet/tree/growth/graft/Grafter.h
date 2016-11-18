/*
 * Grafter.h
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * Main class for grafting. Grafter both provides the selection of the best rootstock (i.e., the 
 * basis of the final tree before grafting) and the actual grafting process. It also provides the 
 * required methods to handle scrapped subnets. The goal of having such a class rather than just 
 * re-using the code of TreeNET Reader is to be able, if needed, to let the user decide if (s)he 
 * wants to save scrapped subnets or even to let the user select the rootstock.
 */

#ifndef GRAFTER_H_
#define GRAFTER_H_

#include "GrafterGrower.h"
#include "BadInputException.h"

class Grafter
{
public:

    // Constructor, destructor
    Grafter(TreeNETEnvironment *env, list<string> setList);
    ~Grafter(); // Implicitely virtual
    
    // Select the adequate trunk
    void selectRootstock();
    
    // Produces the final tree
    Soil* growAndGraft();
    
    // Checks and saves scrapped subnets
    inline bool hasScrappedSubnets() { return this->scrappedSubnets.size() > 0; }
    void outputScrappedSubnets(string filename);

private:

    // Main fields
    TreeNETEnvironment *env;
    unsigned short nbSets;
    string *filePaths;
    SubnetSiteSet **buildingSets;
    
    // Index (in buildingSets) of the set used for the main trunk (+ size of this trunk)
    unsigned short mainSetIndex;
    unsigned short mainTrunkSize;
    
    // List of "scrapped" subnets (i.e., could not be grafted at all)
    list<SubnetSite*> scrappedSubnets;
    
    /*
     * N.B.: it would be tempting to create a new Climber class to implement operations such as 
     * the measurement of the main trunk. However, since these operations are both very specific 
     * (i.e., only relevant to grafting) and simple, they are made available as private methods in 
     * this class.
     */
    
    // Private methods to evaluate trunk and late interfaces of a given tree.
    unsigned short getTrunkSize(NetworkTree *tree);
    bool isTrunkIncomplete(NetworkTree *tree);
    list<InetAddress> listLateInterfaces(NetworkTree *tree);
    void listLateInterfacesRecursive(NetworkTreeNode *cur, list<InetAddress> *res);
    
    // Private methods nuffily leaves at the end of a network tree.
    void nullifyLeaves(NetworkTree *tree);
    void nullifyLeavesRecursive(NetworkTreeNode *cur);
    
};

#endif /* GRAFTER_H_ */
