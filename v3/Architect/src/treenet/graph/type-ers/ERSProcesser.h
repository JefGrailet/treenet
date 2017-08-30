/*
 * ERSProcesser.h
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * This class is made to turn a network tree into a double bipartite graph "Ethernet Switch - 
 * Router - Subnet" just like TreeNET Reader allows to generate since 2015 (with some improvements 
 * since then, notably made in January 2016). It therefore uses both a ERGraph (for the Ethernet 
 * switch - Router part) and a RSGraph (Router - Subnet). It also allows to project the whole 
 * graph on the routers to get a router graph.
 */

#ifndef ERSPROCESSER_H_
#define ERSPROCESSER_H_

#include "../../TreeNETEnvironment.h"
#include "../../tree/Soil.h"
#include "ERGraph.h"
#include "RSGraph.h"

class ERSProcesser
{
public:

    ERSProcesser(TreeNETEnvironment *env);
    ~ERSProcesser();
    
    void process(Soil *fromSoil);
    void output(string filename);
    
    /*
     * check() returns a percentage of vertices being visited when travelling the graph starting 
     * from R1. Ideally, it should return 100% of covered vertices (single connected component). 
     * Otherwise, this should help calling code to advertise the user that there are several 
     * connected components (with the main one having a certain coverage). check() should be 
     * called after process(), and before output().
     */
    
    double check();
    
    void outputSubnetProjection(string filename);
    void outputRouterProjection(string filename);

protected:

    // Reference to environment (can be useful to get parameters/output stream)
    TreeNETEnvironment *env;

    // Reference to the tree being transformed (during a call to process)
    Soil *soilRef;
    
    /*
     * Will contain the resulting bipartite graphs after a call to process(). It should be emptied 
     * after a call to output().
     */
    
    ERGraph *ERResult;
    RSGraph *RSResult;

    /*
     * Arrays used for checking the graph is made of a single connected component (via the check() 
     * method). These fields are useful for the visit() private method (see below) and are only 
     * set for it.
     */
    
    unsigned int nESwitches, nRouters, nSubnets;
    bool *checkArrE, *checkArrR, *checkArrS;
    
    /*
     * In addition, there is also two arrays to map the routers from the ER graph with those from 
     * the RS graph, so that going from one bipartite graph to another can be done quickly.
     */
     
    L3Device **routersER, **routersRS;

    /*
     * Method to travel through the main trunk until we meet a node with multiple children. 
     * Turning that trunk into a bipartite representation is not very interesting. The method 
     * will immediately call processRecursive() (see below) afterwards.
     */
    
    void skipTrunkAndStart(NetworkTree *tree);

    /*
     * Methods to recursively process the tree into a bipartite representation, node by node. 
     * There are two methods, one for the RS part and another one for the ER graph: the idea is 
     * that the ER bipartite graph will use copies of the routers inserted in the RS graph (which 
     * is more complex to generate, hence why it is created first) to keep the same labels. The 
     * depth parameter is useful to check the last route step to subnets at an hedera.
     */
    
    void processRecursiveRS(NetworkTreeNode *cur, unsigned short depth);
    void processRecursiveER(NetworkTreeNode *cur, unsigned short depth);
    
    /*
     * Different methods used during the processing of the tree (RS side). The goal is to isolate 
     * the different operations from each other to keep the processRecursiveRS() method readable 
     * while reducing code redundancy.
     */
    
    bool areRelated(Router *r, SubnetSite *ss);
    
    void connectToRelatedSubnets(Router *r, list<SubnetSite*> *subnetList);
    void connectRemainingSubnets(NetworkTreeNode *n, list<SubnetSite*> *subnetList);
    
    void connectToChildInternal(Router *r, list<SubnetSite*> closeSubnets, NetworkTreeNode *child);
    void connectToChildInternal(NetworkTreeNode *p, list<SubnetSite*> closeSubnets, NetworkTreeNode *child);
    
    /*
     * Method to select the right labels to get all ingress routers of a child internal node. 
     * When this internal has only one label, it is trivial; however, if it is an hedera, one has 
     * to remove from the label list redundant labels, i.e., labels which appear on an inferred 
     * router which already bears a previous label.
     *
     * @param NetworkTreeNode* child   A pointer to the neighborhood node which the labels should 
     *                                 be filtered
     * @return list<InetAdress>        The filtered labels
     */

    list<InetAddress> filterLabels(NetworkTreeNode *child);
    
    /*
     * Method to connect an internal node with a subnet that is just above in the tree (hence the 
     * "child") which is normally crossed to reach the corresponding neighborhood. routerChild 
     * parameter is present simply to avoid looking up again for the router of "child" that bears 
     * "labelChild".
     *
     * @param list<SubnetSite*> closeSubnets   Candidate subnets to achieve the connection
     * @param NetworkTreeNode*  child          The neighborhood node to connect with a subnet
     * @param InetAddress       labelChild     The label of the same node which the connection 
     *                                         will be based on
     * @param Router*           routerChild    The router of "child" bearing "labelChild" (can be 
     *                                         NULL; in that case, will be replaced with an 
     *                                         imaginary router)
     * @return bool                            True if a new edge was created, false otherwise
     */
    
    bool connectToCloseSubnet(list<SubnetSite*> closeSubnets, 
                              NetworkTreeNode *child, 
                              InetAddress labelChild, 
                              Router *routerChild);
    
    /*
     * Method to get a "connecting subnet" for a given neighborhood, based on the label through 
     * which the connection should exist. Because of route stretching and other routing dynamics, 
     * it is possible the subnet which contains the label is located somewhere else in the tree. 
     * This method looks for it and also ensures this connecting subnet is not a child leaf of 
     * "child" (otherwise the connection medium between "child" and its parent neighborhood should 
     * be an imaginary subnet).
     *
     * @param NetworkTreeNode* child       The neighborhood which should be connected to the 
     *                                     connecting subnet
     * @param InetAddress      labelChild  The label which should be contained by the connecting 
     *                                     subnet
     * @return SubnetSite*                 The subnet that was found, if valid (NULL otherwise)
     */
    
    SubnetSite* getValidConnectingSubnet(NetworkTreeNode *child, InetAddress labelChild);
    
    // Method to visit a node and visits its neighbors except if already visited.
    void visit(Vertice *node);

};

#endif /* ERSPROCESSER_H_ */
