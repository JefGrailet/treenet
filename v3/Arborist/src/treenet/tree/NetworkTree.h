/*
 * NetworkTree.h
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
 *
 * NetworkTree is a custom kind of tree that models the DAG (directed acyclic graph) made of the
 * previousfly inferred (and refined) subnets where the leaves are the subnets and the internal
 * nodes are the neighborhoods (i.e. network areas where every device is reachable with at most 
 * one hop). Each node is labeled with an interface which is the responding interface when 
 * sending an ICMP probe to some target subnet with a smaller TTL than required. In other words,
 * for a given subnet located at N hops, one will have to go through N internal nodes in the
 * tree to reach the subnet.
 *
 * While what we obtain with routes can usually be viewed as a tree, it is still possible to 
 * have a DAG because of load balancing (i.e. branches reaching a same point but using different 
 * routes). Construction method ensures that routes bearing differences for a same destination 
 * (because of load balancing, for example) are fused into a single branch (January 2015).
 *
 * The main goal of building such tree is to locate subnets between themselves and to have a good
 * approximation of the number of interfaces bordering a neighborhood (which is necessary for 
 * L2/L3 devices inference).
 *
 * In September 2016, this class was heavily modified to move all its main operations in other 
 * classes for the sake of clarity (original NetworkTree class started to get very verbose). 
 * Moreover, a new feature will allow the creation of several trees rather than a single unified 
 * tree for a same network.
 */

#ifndef NETWORKTREE_H_
#define NETWORKTREE_H_

#include <string>
using std::string;

#include "NetworkTreeNode.h"

class NetworkTree
{
public:

    // Constructor, destructor
    NetworkTree();
    ~NetworkTree();
    
    // Accesser to the root node
    inline NetworkTreeNode *getRoot() { return this->root; }

    // Method to write the subnets/leaves in a string object (later written in a file in Soil).
    string writeSubnets();
    
private:

    // Private fields: root and "root offset" (0 = root is the vantage point)
    NetworkTreeNode *root;
    unsigned short rootOffset;
    
    // Recursive method to list the subnets (used for writeSubnets())
    static void listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur);
    
};

#endif /* NETWORKTREE_H_ */
