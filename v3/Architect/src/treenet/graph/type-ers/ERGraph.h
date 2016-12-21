/*
 * ERGraph.h
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * This child of BipartiteGraph models a bipartite graphs where the parties are the inferred 
 * routers (right party) and the virtual Ethernet switches (left party) that connect them in order 
 * to have all routers of a same neighborhood connected together. The class has been named 
 * "ERGraph" ("ER" standing for "Ethernet switch - Router") for convenience. It constitutes one of 
 * the bipartite graphs of an ERS double bipartite graph (ERS = "Ethernet - Router - Subnet").
 */

#ifndef ERGRAPH_H_
#define ERGRAPH_H_

#include <map>
using std::map;
using std::pair;

#include "../L2Device.h"
#include "RSGraph.h"

class ERGraph : public BipartiteGraph
{
public:

    ERGraph();
    ~ERGraph();
    
    /*
     * Method to copy the routers obtained in an RS graph. As the ER graph is currently used for 
     * a double bipartite graph and not a as a "stand-alone" graph, it is necessary that it gets 
     * the same routers as in the RS graph, otherwise both graphs will not match and will not 
     * properly form the double bipartite ERS graph which is expected in the end.
     */
    
    void copyRouters(RSGraph *bipRS);
    
    // Method to connect a given set of routers together via a virtual switch.
    void connectRouters(list<Router*> routers, NetworkTreeNode *withImaginary);
    
    // Method to connect all routers of a given neighborhood via a virtual switch.
    void linkAllRouters(NetworkTreeNode *neighborhood);

protected:
    
    /*
     * Maps to keep track of routers already present in the graph. Note that there is currently no 
     * map for the switches, because they are created "on the fly" and directly connected to all 
     * routers they should be connected to. Besides, there is no "structure" equivalent to achieve 
     * an accurate mapping (hedera's can have several switches).
     */
    
    map<NetworkTreeNode*, L3Device*> imaginaryRouters;
    map<Router*, L3Device*> routers;
    
    // Method to create a new switch in the graph. Returns the created element.
    L2Device* createSwitch();
    
    // Methods to retrieve an inferred or imaginary router in the graph (return NULL if absent)
    L3Device* lookUp(Router *router);
    L3Device* lookUpImaginary(NetworkTreeNode *neighborhood); // For imaginary router
    
};

#endif /* ERGRAPH_H_ */
