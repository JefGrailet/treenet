/*
 * Edge.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Models an edge in a graph (any type). It can connect together any type of vertices.
 */

#ifndef EDGE_H_
#define EDGE_H_

// Inclusion of Vertice with forward declaration
#include "Vertice.h"
class Vertice;

class Edge
{
public:

    Edge(Vertice *v1, Vertice *v2);
    ~Edge();
    
    inline Vertice* getVerticeOne() { return v1; }
    inline Vertice* getVerticeTwo() { return v2; }
    
    // Checks if this edge is incident to some vertice
    bool connects(Vertice *v);

    string toString();
    
    static bool smaller(Edge *e1, Edge *e2);

protected:

    Vertice *v1, *v2;
    
};

#endif /* EDGE_H_ */
