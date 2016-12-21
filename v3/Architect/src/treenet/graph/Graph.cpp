/*
 * Graph.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Graph.h (see this file to learn further about the goals of 
 * such class).
 */

#include <sstream>
using std::stringstream;

#include "Graph.h"

Graph::Graph()
{
}

Graph::~Graph()
{
    for(list<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it)
    {
        delete (*it);
    }
    edges.clear();
}

void Graph::createEdge(Vertice *v1, Vertice *v2)
{
    if(!v1->isConnectedTo(v2))
    {
        Edge *newEdge = new Edge(v1, v2);
        edges.push_back(newEdge);
        
        v1->addIncidentEdge(newEdge);
        v2->addIncidentEdge(newEdge);
    }
}

string Graph::edgesToString()
{
    stringstream ss;
    for(list<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it)
    {
        ss << (*it)->toString() << "\n";
    }
    return ss.str();
}
