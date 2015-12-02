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
 */

#ifndef NETWORKTREE_H_
#define NETWORKTREE_H_

#include "SubnetSite.h"
#include "NetworkTreeNode.h"
#include "../aliasresolution/AliasHintCollector.h"
#include "../aliasresolution/AliasResolver.h"

class NetworkTree
{
public:

    /*
     * Size of the subnetMap array (array of lists), which is used for fast look-up of a subnet 
     * stored in the tree on the basis of an interface it should contain. Its size is base on the 
     * fact that no subnet of a prefix length shorter than /20 was ever found in the measurements. 
     * Therefore, the 20 first bits of any interface in a subnet are used to access a list of 
     * subnets sharing the same prefix in O(1), the list containing at most 2048 subnets (2048 
     * subnets of prefix length /31). This dramatically speeds up the look-up for a subnet stored 
     * in the tree (in comparison with a more trivial method where one visits the tree), at the 
     * cost of using more memory).
     */

    const static int SIZE_SUBNET_MAP = 1048576;

    /*
     * About maxDepth parameter: it is the size of the longest route to a subnet which should be 
     * inserted in the tree. It is used as the size of the depthMap array (i.e. one list of nodes 
     * per depth level), which should be maintained throughout the life of the tree to ease the 
     * insertion step (re-building the whole map at each insertion is costly).
     */

    NetworkTree(unsigned short maxDepth);
    ~NetworkTree();
    
    // Insertion method
    void insert(SubnetSite *subnet);
    
    // Prints the tree (policy: depth-first pre-order)
    void visit(ostream *out);
    
    // Completes the route to a given subnet with information present in the tree.
    void repairRoute(SubnetSite *ss);
    
    // Visits each internal node to collect alias resolution hints
    void collectAliasResolutionHints(ostream *out, AliasHintCollector *ahc);
    
    // Gets a subnet contained in the tree which contains the given input address.
    SubnetSite *getSubnetContaining(InetAddress needle);
    
    // Prints the interesting internal nodes (i.e. more than one child that is not an internal)
    void internals(ostream *out, AliasResolver *ar);
    
    // Method to write the subnets/leaves in an output file of a given name.
    void outputSubnets(string filename);
    
private:

    /*
     * About depth parameter (insertSubnet() and insertRecursive()): it is the depth at which we
     * are in the tree, which we need to know in order to find the appropriate label(s) in the
     * subnet route array.
     */

    // Method to actually insert a subnet (with a cascade of internal nodes if needed)
    static NetworkTreeNode *insertSubnet(SubnetSite *subnet, unsigned short depth);

    // Recursive methods to go through the tree (with or without insertion)
    static void insertRecursive(SubnetSite *subnet, NetworkTreeNode *cur, unsigned short depth);
    static void visitRecursive(ostream *out, NetworkTreeNode *cur, unsigned short depth);
    static void collectHintsRecursive(ostream *out, 
                                      AliasHintCollector *ahc, 
                                      NetworkTreeNode *cur, 
                                      unsigned short depth);
    static void listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur);
    static void internalsRecursive(ostream *out, 
                                   NetworkTree *tree, 
                                   NetworkTreeNode *cur, 
                                   AliasResolver *ar);
      
    /*
     * Recursive method (but going back up in the tree) to prune a branch which last node has
     * no leaf and is not a T_SUBNET node and which all intermediate nodes have a single child.
     * The depth map (map parameter) and the depth metric are necessary, because we have to 
     * remove each deleted node from the depth map as well.
     */
    
    static void prune(list<NetworkTreeNode*> *map, 
                      NetworkTreeNode *cur, 
                      NetworkTreeNode *prev,
                      unsigned short depth);

    /*
     * Private fields: root of the tree, depth map (for insertion), subnet map (for look up) along 
     * its size.
     */
    
    NetworkTreeNode *root;
    list<NetworkTreeNode*> *depthMap;
    list<SubnetSite*> *subnetMap;
    unsigned short maxDepth;
};

#endif /* NETWORKTREE_H_ */
