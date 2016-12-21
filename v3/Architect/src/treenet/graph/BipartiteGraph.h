/*
 * BipartiteGraph.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * This child of Graph models a graph where there are two kinds of vertice (hence the name 
 * "BipartiteGraph"). It further extends into other children classes, like SimpleGraph.
 */

#ifndef BIPARTITEGRAPH_H_
#define BIPARTITEGRAPH_H_

#include "Graph.h"

class BipartiteGraph : public Graph
{
public:

    BipartiteGraph();
    ~BipartiteGraph();
    
    // Methods for parties (methods to create vertices should be defined in children classes)
    inline list<Vertice*> *getPartyOne() { return &this->partyOne; }
    inline list<Vertice*> *getPartyTwo() { return &this->partyTwo; }
    
    string partyOneToString();
    string partyTwoToString();
    
    // Computes some metrics in string format
    string getMetricsString(bool showUnconnected = true);

protected:

    list<Vertice*> partyOne;
    list<Vertice*> partyTwo;
    
    // Counters to assign IDs to each element
    unsigned int counterPartyOne, counterPartyTwo;
};

#endif /* BIPARTITEGRAPH_H_ */
