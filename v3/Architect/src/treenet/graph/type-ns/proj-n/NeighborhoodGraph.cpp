/*
 * NeighborhoodGraph.cpp
 *
 *  Created on: Dec 1, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in NeighborhoodGraph.h (see this file to learn further about the 
 * goals of such class).
 */

#include "NeighborhoodGraph.h"

NeighborhoodGraph::NeighborhoodGraph(NSGraph *toProject) : SimpleGraph()
{
    // Makes a copy-construction of each listed neighborhood (no need for sorting)
    list<Vertice*> *neighborhoods = toProject->getPartyOne();
    for(list<Vertice*>::iterator it = neighborhoods->begin(); it != neighborhoods->end(); ++it)
    {
        Neighborhood *curN = (Neighborhood*) (*it);
        Neighborhood *copy = new Neighborhood(curN);
        this->vertices.push_back(copy);
        this->copiedNeighborhoods.insert(pair<string, Neighborhood*>(curN->toString(), copy));
    }

    // Creates the new edges by looking at the subnet side
    list<Vertice*> *subnets = toProject->getPartyTwo();
    for(list<Vertice*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        Subnet *curSubnet = (Subnet*) (*it);
        
        list<Edge*> *incidentEdges = curSubnet->getIncidentEdges();
        for(list<Edge*>::iterator i = incidentEdges->begin(); i != incidentEdges->end(); ++i)
        {
            // Neighborhood is always first vertice
            Neighborhood *n1 = this->getEquivalent((Neighborhood*) (*i)->getVerticeOne());
            if(n1 == NULL)
                continue;
            
            // Starting at i + 1 ensures no duplicate edge
            for(list<Edge*>::iterator j = ++i; j != incidentEdges->end(); ++j)
            {
                Neighborhood *n2 = this->getEquivalent((Neighborhood*) (*j)->getVerticeOne());
                if(n2 == NULL)
                    continue;
                
                Graph::createEdge(n1, n2);
            }
            i--; // To "fix" next iteration
        }
    }
    
    Graph::sortEdges();
}

NeighborhoodGraph::~NeighborhoodGraph()
{
    // Nothing, the different items will be deleted by parent constructors
}

Neighborhood* NeighborhoodGraph::getEquivalent(Neighborhood *original)
{
    map<string, Neighborhood*>::iterator res = copiedNeighborhoods.find(original->toString());
    if(res != copiedNeighborhoods.end())
        return res->second;
    return NULL;
}
