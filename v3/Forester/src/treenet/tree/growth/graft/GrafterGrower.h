/*
 * GrafterGrower.h
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * This grower is a slight variant of the ClassicGrower. It is essentially the same class and 
 * algorithms, except it does not provide repairment, and there is no preparation operation 
 * either. It however provides grafting methods, and as a consequence, is solely used by the 
 * Grafter class.
 */

#ifndef GRAFTERGROWER_H_
#define GRAFTERGROWER_H_

#include "../Grower.h"

class GrafterGrower : public Grower
{
public:

    // Constructors, destructor
    GrafterGrower(TreeNETEnvironment *env);
    GrafterGrower(TreeNETEnvironment *env, unsigned short maxDepth);
    ~GrafterGrower(); // Implicitely virtual
    
    // N.B.: the second constructor is meant for the final tree of a grafting process.
    
    void prepare(); // Implicitely virtual
    void grow(); // Implicitely virtual
    
    // Grafting method (returns true if successful, false if grafting is impossible)
    bool graft(SubnetSite *ss);
    
    // Method to flush the map entries at the end of grafting
    void flushMapEntries();

protected:

    // Depth map for tree construction, and the tree being grown
    unsigned short maxDepth;
    list<NetworkTreeNode*> *depthMap;
    NetworkTree *tree;
    
    // List of new subnet map entries (will be moved to the map in a Soil object)
    list<SubnetMapEntry*> newSubnetMapEntries;
    
    // Protected methods that are strictly identical to their equivalent in ClassicGrower class
    void insert(SubnetSite *subnet);
    void prune(NetworkTreeNode *cur, NetworkTreeNode *prev, unsigned short depth);
    static NetworkTreeNode *createBranch(SubnetSite *subnet, unsigned short depth);
    
    // Method checking if the route of some subnet matches the trunk of the current tree
    bool matchesTrunk(SubnetSite *subnet);
    
    /*
     * Method to study if a given subnet can be grafted by adapting the route to it.
     *
     * @param SubnetSite*      ss         The subnet to insert
     * @param unsigned short*  sOld       The size of the array "oldPrefix" (see below)
     * @param InetAddress**    oldPrefix  The route prefix that should be replaced
     * @param unsigned short*  sNew       The size of the array "newPrefix" (see below)
     * @param InetAddress**    newPrefix  The route prefix that should replace "oldPrefix"
     * @return bool                       True if the subnet can be grafted, false otherwise
     */
    
    bool isGraftable(SubnetSite *ss, 
                     unsigned short *sOld, 
                     InetAddress **oldPrefix, 
                     unsigned short *sNew, 
                     InetAddress **newPrefix);

};

#endif /* GRAFTERGROWER_H_ */
