/*
 * NetworkTreeNode.h
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it simply represents a node in a "network tree".
 * There are two kinds of nodes: the nodes that model a neighborhood (T_NEIGHBORHOOD) and the 
 * nodes that represent subnets (T_SUBNET). These types match the distinction between internal 
 * nodes (neighborhood nodes) and leaves (subnet nodes). There is a 3rd type of node (T_ROOT)
 * which is only used for the root of a network tree.
 *
 * Each node is labelled with an IP, which is the IP of the interface responding to an ICMP probe 
 * which targets a destination that is further in the network but which TTL is just enough to
 * reach that part of the network/neighborhood. Subnet nodes are labelled with the contra-pivot
 * of the associated subnet.
 *
 * Since January 2015, multiple labels are allowed for nodes to deal with route differences, e.g. 
 * caused by load balacing or route flickering. Nodes where one interface varies from one route to 
 * another (all these routes reaching the same destination) are labelled with all IPs found in the 
 * routes we used, such that that we do not create multiple branches for all routes reaching a 
 * same location, which would result in misinterpretating the actual network.
 */

#ifndef NETWORKTREENODE_H_
#define NETWORKTREENODE_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "SubnetSite.h"
#include "InvalidSubnetException.h"

class NetworkTreeNode
{
public:

    // Types of node
    enum NodeType
    {
        T_ROOT, 
        T_SUBNET, 
        T_NEIGHBORHOOD, 
        T_HEDERA
    };
    
    /*
     * T_HEDERA denotes an internal node which bears several labels, which is a consequence of 
     * routing irregularities. When a node gets more than one label (see how construction works), 
     * there are two possibilities:
     * -either the labels belong to separate routers which nevertheless still lead to a same 
     *  network location, then it means these routers are part of a QoS strategy, 
     * -either the labels actually belong to a same router; simply, there are different entry 
     *  ports to it, and the phenomenon is mostly caused by a previous QoS-engineered zone or 
     *  route flickering.
     * In the first case, we have a superposition of Neighborhoods in a same node, while in the 
     * second, we have a single Neighborhood but which is most likely a consequence of a former 
     * superposition.
     *
     * Such nodes therefore have been denoted as "Hedera" nodes. Hedera comes from the Latin verb 
     * "haerere" ("to be tied to") and later inspired the French word "lierre", i.e. a kind of 
     * ivy. This name has been chosen because the Hedera nodes "ties" what would otherwise be 
     * several distinct branches together into a single one, just like real-life ivies can wrap 
     * around distinct branches of a tree.
     * 
     * Sources: 
     * https://en.wikipedia.org/wiki/Hedera
     * https://fr.wikipedia.org/wiki/Hedera (about Latin root)
     */

    NetworkTreeNode(); // Will build a root node
    NetworkTreeNode(InetAddress label);
    NetworkTreeNode(SubnetSite *subnet) throw (InvalidSubnetException);
    ~NetworkTreeNode();
    
    // Accessors
    inline unsigned short getType() const { return type; }
    inline NetworkTreeNode *getParent() { return parent; }
    inline SubnetSite *getAssociatedSubnet() { return associatedSubnet; }
    inline list<InetAddress> *getLabels() { return &labels; }
    inline list<InetAddress> *getPreviousLabels() { return &previousLabels; }
    inline list<NetworkTreeNode*> *getChildren() { return &children; }
    
    // Setter
    inline void setParent(NetworkTreeNode *p) { this->parent = p; }
    
    /*
     * Methods to handle labels/types:
     * -to check if this node is the root (T_ROOT)
     * -to check if this node is a leave (T_SUBNET)
     * -to check if this node is an internal (T_NEIGHBORHOOD or T_HEDERA)
     * -to check if this node is a Hedera node (= multiple labels)
     * -to check if a given label is among the multiple labels of this node
     * -to add a label
     * -to add a previous label occuring in the routes of the subnets at the end of this branch 
     *  (useful for multi-labels nodes analysis and disambiguation)
     */
    
    bool isRoot();
    bool isLeaf();
    bool isInternal();
    bool isHedera();
    bool hasLabel(InetAddress label);
    bool hasPreviousLabel(InetAddress label);
    void addLabel(InetAddress label);
    void addPreviousLabel(InetAddress label);
    
    // Static comparison method for sorting purposes
    static bool compare(NetworkTreeNode *n1, NetworkTreeNode *n2);
    
    // Inline method to (re-)sort the children
    inline void sortChildren() { children.sort(NetworkTreeNode::compare); }
    
    // Method to add a child to this node
    void addChild(NetworkTreeNode *child);
    
    // Method to merge children from a given node to this one
    void merge(NetworkTreeNode *mergee);
    
    // Method to get a child of this node, given a label (returns NULL if no such child)
    NetworkTreeNode *getChild(InetAddress label);
    
    // Boolean method to know if the current node has subnets (leaves) as children
    bool hasLeavesAsChildren();
    
    /*
     * Method to check if all non-subnet children have their label belonging to a brother 
     * subnet. An internal node satisfying this property is said to have complete linkage. For 
     * this case, the method return 0. When there is exactly one missing link, the method returns 
     * 1, and it will return 2 if there are more than 2 missing links.
     */

    unsigned short getLinkage();
    
    /*
     * List the interfaces of this node if internal. The listed interfaces are:
     * -the interfaces listed as labels,
     * -the contra-pivots of the children subnets.
     * It should be noted that the list does not exactly have the number of inferred interfaces 
     * during internals analysis. It should be identical, however, for cases where there is only 
     * one label and only accurate and credible subnets as children.
     */
    
    list<InetAddress> listInterfaces();
    
private:

    // Label(s), type, associated subnet (if any) of the node
    list<InetAddress> labels;
    unsigned short type;
    SubnetSite *associatedSubnet;

    // Children are stored in a list
    list<NetworkTreeNode*> children;
    
    // Parent node is maintained too
    NetworkTreeNode *parent;
    
    /*
     * A list is also used to maintain the previous label(s) in the route to the subnets ending 
     * this branch. This helps to analyze multi-labels nodes.
     */
    
    list<InetAddress> previousLabels;
};

#endif /* NETWORKTREENODE_H_ */
