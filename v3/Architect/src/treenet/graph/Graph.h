/*
 * Graph.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Graph is a superclass used to model the different types of graphs (regular or bipartite) 
 * produced by TreeNET "Architect". It defines the common feature of all such graphs: it 
 * consists of a list of edges.
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include "Edge.h"

class Graph
{
public:

    Graph();
    ~Graph();
    
    // General methods to handle edges
    void createEdge(Vertice *v1, Vertice *v2);
    inline void sortEdges() { edges.sort(Edge::smaller); }
    inline list<Edge*> *getEdges() { return &this->edges; }
    string edgesToString();
    
    /*
     * No virtual "sortVertices()" (see SimpleGraph and BipartiteGraph classes) because this 
     * apparently annoys the compiler for some reason. Still, fort sortVertices() being defined 
     * in both child classes, this should not be an issue.
     */

protected:

    list<Edge*> edges;
    
    /*
     * N.B.: depending or whether a graph is bipartite or not, one needs to have two list of 
     * vertices or a single one. This is why the only common field for all graphs is the "edges" 
     * field.
     */
    
};

#endif /* GRAPH_H_ */
