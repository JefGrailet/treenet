/*
 * SubnetGraph1.cpp
 *
 *  Created on: Dec 1, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetGraph1.h (see this file to learn further about the goals 
 * of such class).
 */

#include "SubnetGraph1.h"

SubnetGraph1::SubnetGraph1(NSGraph *toProject) : SimpleGraph()
{
    // Makes a copy-construction of each subnet listed in the bipartite graph (no need for sorting)
    list<Vertice*> *subnets = toProject->getPartyTwo();
    for(list<Vertice*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        Subnet *curSubnet = (Subnet*) (*it);
        Subnet *copy = new Subnet(curSubnet);
        this->vertices.push_back(copy);
        this->copiedSubnets.insert(pair<string, Subnet*>(curSubnet->toString(), copy));
    }

    // Creates the new edges by looking at the neighborhood side
    list<Vertice*> *neighborhoods = toProject->getPartyOne();
    for(list<Vertice*>::iterator it = neighborhoods->begin(); it != neighborhoods->end(); ++it)
    {
        Neighborhood *curN = (Neighborhood*) (*it);

        list<Edge*> *incidentEdges = curN->getIncidentEdges();
        for(list<Edge*>::iterator i = incidentEdges->begin(); i != incidentEdges->end(); ++i)
        {
            // Subnet is always second vertice
            Subnet *subnet1 = this->getEquivalent((Subnet*) (*i)->getVerticeTwo());
            if(subnet1 == NULL)
                continue;

            // Starting at i + 1 ensures no duplicate edge
            for(list<Edge*>::iterator j = ++i; j != incidentEdges->end(); ++j)
            {
                Subnet *subnet2 = this->getEquivalent((Subnet*) (*j)->getVerticeTwo());
                if(subnet2 == NULL)
                    continue;
                
                Graph::createEdge(subnet1, subnet2);
            }
            i--; // To "fix" next iteration
        }
    }
    Graph::sortEdges();
}

SubnetGraph1::~SubnetGraph1()
{
    // Nothing, the different items will be deleted by parent constructors
}

Subnet* SubnetGraph1::getEquivalent(Subnet *original)
{
    map<string, Subnet*>::iterator res = copiedSubnets.find(original->toString());
    if(res != copiedSubnets.end())
        return res->second;
    return NULL;
}
