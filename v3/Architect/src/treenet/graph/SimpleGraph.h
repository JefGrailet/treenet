/*
 * SimpleGraph.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * This child of Graph models a graph where there is only one kind of vertice (hence the name 
 * "SimpleGraph"). It further extends into other children classes, like BipartiteGraph.
 */

#ifndef SIMPLEGRAPH_H_
#define SIMPLEGRAPH_H_

#include "Graph.h"

class SimpleGraph : public Graph
{
public:

    SimpleGraph();
    ~SimpleGraph();
    
    // General methods to handle vertices (createVertice() should be defined in a child class)
    inline list<Vertice*> *getVertices() { return &this->vertices; }
    string verticesToString();
    
    // Computes some metrics in string format
    string getMetricsString();

protected:

    list<Vertice*> vertices;
};

#endif /* SIMPLEGRAPH_H_ */
