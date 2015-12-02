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
 * caused by load balacing. Nodes where one interface varies from one route to another (all these 
 * routes reaching the same destination) are labelled with all IPs found in the routes we used, 
 * such that that we do not create multiple branches for all routes reaching a same location, 
 * which would result in misinterpretating the actual network.
 */

#ifndef NETWORKTREENODE_H_
#define NETWORKTREENODE_H_

#include <list>
using std::list;

#include "../common/inet/InetAddress.h"
#include "SubnetSite.h"
#include "Router.h"
#include "InvalidSubnetException.h"
#include "../bipartite/BipartiteRouter.h"

class NetworkTreeNode
{
public:

    // Types of node
    enum NodeType
    {
        T_NEIGHBORHOOD,
        T_SUBNET,
        T_ROOT
    };

    NetworkTreeNode(); // Will build a root node
    NetworkTreeNode(InetAddress label);
    NetworkTreeNode(SubnetSite *subnet) throw (InvalidSubnetException);
    ~NetworkTreeNode();
    
    /*
     * Max difference between 2 IP identifiers (for router inference) to associate their 
     * respective interfaces during router inference. Actually, it is adapted during inference, 
     * but this constant is a good "basis".
     *
     * Also, another static variable defines the threshold (for the gap between IP Ids) to not 
     * associate interfaces and represent each interface with a separate router.
     */
    
    static const unsigned short MAX_IP_ID_DIFFERENCE = 50;
    static const unsigned short NO_ASSOCIATION_THRESHOLD = 250;
    
    // Accessors
    inline unsigned short getType() const { return type; }
    inline NetworkTreeNode *getParent() { return parent; }
    inline SubnetSite *getAssociatedSubnet() { return associatedSubnet; }
    inline list<InetAddress> *getLabels() { return &labels; }
    inline list<InetAddress> *getPreviousLabels() { return &previousLabels; }
    inline list<NetworkTreeNode*> *getChildren() { return &children; }
    inline list<Router*> *getInferredRouters() { return &inferredRouters; }
    
    // Setter
    inline void setParent(NetworkTreeNode *p) { this->parent = p; }
    
    /*
     * Methods to handle labels:
     * -check if the node has multiple labels (= load balancer)
     * -check if a given label is among the multiple labels of this node
     * -add a label
     * -add a previous label occuring in the routes of the subnets at the end of this branch 
     *  (useful for multi-labels nodes analysis and disambiguation)
     */
    
    bool isLoadBalancer();
    bool hasLabel(InetAddress label);
    bool hasPreviousLabel(InetAddress label);
    void addLabel(InetAddress label);
    void addPreviousLabel(InetAddress label);
    
    // Static comparison method for sorting purposes
    static bool compare(NetworkTreeNode *n1, NetworkTreeNode *n2);
    
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
    
    // Method to obtain the degree of the current node (exclusive to TreeNET reader for now)
    unsigned int getDegree();
    
    /*
     * List the interfaces of this node if neighborhood. The listed interfaces are:
     * -the interfaces listed as labels,
     * -the contra-pivots of the children subnets.
     * It should be noted that the list does not exactly have the number of inferred interfaces 
     * during neighborhood analysis. It should be identical, however, for cases where there is no
     * load balancing and only accurate and credible subnets as children. Also, pointers are 
     * listed rather than InetAddress in order to get the probe token/IP identifiers of each 
     * interface (more precisely, to copy the InetAddress objects with these pieces of data).
     */
    
    list<InetAddress*> listInterfaces();
    
    /*
     * Method to copy the probe token/IP identifier of the label(s) of the internal nodes leading 
     * to a leaf into the route of the associated subnet (if the label(s) indeed has/have an 
     * associated IP identifier. These details are labeled as "router info" for simplicity.
     */
    
    void copyRouterInfoIntoRoute();
    
    /*
     * Method to infer routers within a neighborhood. The alias resolution method used combines 
     * both reverse DNS and a technique relying on IP Identifiers (which is similar to what is 
     * used in Radar Gun). The method returns a list of routers, themselves being lists of 
     * interfaces. Also, the method only return what we are able to infer; i.e. if we are missing 
     * interfaces the list will be obviously incomplete.
     *
     * N.B.: unlike TreeNET, this method is void because the list of router is stored in the
     * node for further processing (i.e. conversion into bipartite graph).
     */
    
    void inferRouters();
    
    // Method to get the inferred router having a given interface. Returns NULL if no router.
    Router* getRouterHaving(InetAddress interface);
    
    // Accessor/setter for the (potential) imaginary router for this node in the bipartite graph.
    inline BipartiteRouter *getImaginaryRouter() { return imaginaryRouter; }
    inline void setImaginaryRouter(BipartiteRouter *ir) { this->imaginaryRouter = ir; }
    
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
    
    /*
     * Exclusive to TreeNET reader: list of routers of this node after L3 inference. Such a list 
     * is later used to build a bipartite graph of the measured (and inferred) topology.
     */
    
    list<Router*> inferredRouters;
    
    /*
     * Still exclusive to TreeNET reader: a pointer to the bipartite element representing the 
     * ingress router, when this router had to be "imagined".
     */
    
    BipartiteRouter *imaginaryRouter;
};

#endif /* NETWORKTREENODE_H_ */
