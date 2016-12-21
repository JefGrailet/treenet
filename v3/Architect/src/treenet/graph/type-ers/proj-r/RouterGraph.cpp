/*
 * RouterGraph.cpp
 *
 *  Created on: Dec 14, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in RouterGraph.h (see this file to learn further about the goals 
 * of such class).
 */

#include "RouterGraph.h"

RouterGraph::RouterGraph(ERGraph *toProject1, RSGraph *toProject2) : SimpleGraph()
{
    // Makes a copy-construction of each router listed in the first graph (no need for sorting)
    list<Vertice*> *routers = toProject1->getPartyTwo();
    for(list<Vertice*>::iterator it = routers->begin(); it != routers->end(); ++it)
    {
        L3Device *curRouter = (L3Device*) (*it);
        L3Device *copy = new L3Device(curRouter);
        this->vertices.push_back(copy);
        this->copiedRouters.insert(pair<string, L3Device*>(curRouter->toString(), copy));
    }

    // Creates part of the new edges by looking at the ethernet switch side (1st graph)
    list<Vertice*> *switches = toProject1->getPartyOne();
    for(list<Vertice*>::iterator it = switches->begin(); it != switches->end(); ++it)
    {
        L2Device *curSwitch = (L2Device*) (*it);

        list<Edge*> *incidentEdges = curSwitch->getIncidentEdges();
        for(list<Edge*>::iterator i = incidentEdges->begin(); i != incidentEdges->end(); ++i)
        {
            // Router is always second vertice here
            L3Device *router1 = this->getEquivalent((L3Device*) (*i)->getVerticeTwo());
            if(router1 == NULL)
                continue;

            // Starting at i + 1 ensures no duplicate edge
            for(list<Edge*>::iterator j = ++i; j != incidentEdges->end(); ++j)
            {
                L3Device *router2 = this->getEquivalent((L3Device*) (*j)->getVerticeTwo());
                if(router2 == NULL)
                    continue;
                
                Graph::createEdge(router1, router2);
            }
            i--; // To "fix" next iteration
        }
    }
    
    // Creates the other part of the new edges by looking at the subnet side (2nd graph)
    list<Vertice*> *subnets = toProject2->getPartyTwo();
    for(list<Vertice*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        Subnet *curSubnet = (Subnet*) (*it);

        list<Edge*> *incidentEdges = curSubnet->getIncidentEdges();
        for(list<Edge*>::iterator i = incidentEdges->begin(); i != incidentEdges->end(); ++i)
        {
            // Router is always first vertice here
            L3Device *router1 = this->getEquivalent((L3Device*) (*i)->getVerticeOne());
            if(router1 == NULL)
                continue;

            // Starting at i + 1 ensures no duplicate edge
            for(list<Edge*>::iterator j = ++i; j != incidentEdges->end(); ++j)
            {
                L3Device *router2 = this->getEquivalent((L3Device*) (*j)->getVerticeOne());
                if(router2 == NULL)
                    continue;
                
                Graph::createEdge(router1, router2);
            }
            i--; // To "fix" next iteration
        }
    }
    
    Graph::sortEdges();
}

RouterGraph::~RouterGraph()
{
    // Nothing, the different items will be deleted by parent constructors
}

L3Device* RouterGraph::getEquivalent(L3Device *original)
{
    map<string, L3Device*>::iterator res = copiedRouters.find(original->toString());
    if(res != copiedRouters.end())
        return res->second;
    return NULL;
}
