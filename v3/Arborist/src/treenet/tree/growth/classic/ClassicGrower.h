/*
 * ClassicGrower.h
 *
 *  Created on: Sept 9, 2016
 *      Author: grailet
 *
 * This grower produces a tree following the "historic" tree construction method that was used in 
 * TreeNET v1.0 and TreeNET v2.0+. Its prepare() method consists in running Paris traceroute to 
 * every subnet. Its grow() method is fully passive (i.e., there is no probing at this step) and 
 * exclusively relies on the traceroute results obtained at previous step to build the tree.
 */

#ifndef CLASSICGROWER_H_
#define CLASSICGROWER_H_

#include "../Grower.h"

class ClassicGrower : public Grower
{
public:

    // Constructor, destructor
    ClassicGrower(TreeNETEnvironment *env);
    ~ClassicGrower(); // Implicitely virtual
    
    void prepare(); // Implicitely virtual
    void grow(); // Implicitely virtual

protected:

    // Depth map for tree construction, and the tree being grown
    list<NetworkTreeNode*> *depthMap;
    NetworkTree *tree;
    
    // List of new subnet map entries (will be moved to the map in a Soil object)
    list<SubnetMapEntry*> newSubnetMapEntries;
    
    // Method to insert a subnet in the tree.
    void insert(SubnetSite *subnet);
    
    /*
     * Recursive method (but going back up in the tree) to prune a branch which last node has
     * no leaf and is not a T_SUBNET node and which all intermediate nodes have a single child.
     * The depth map (map parameter) and the depth metric are necessary, because we have to 
     * remove each deleted node from the depth map as well.
     */
    
    void prune(NetworkTreeNode *cur, NetworkTreeNode *prev, unsigned short depth);
    
    // Completes the route to a given subnet with information present in the tree.
    void repairRoute(SubnetSite *ss);
    
    // Static methods to create a branch and to visit one to list subnets (respectively)
    static NetworkTreeNode *createBranch(SubnetSite *subnet, unsigned short depth);
    static void listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur);
    
    /*
     * About depth parameter (createBranch()): it is the depth at which we are in the tree, which 
     * we need to know in order to find the appropriate label(s) in the subnet route array.
     */
    
};

#endif /* CLASSICGROWER_H_ */
