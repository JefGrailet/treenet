/*
 * SubnetGraph2.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetGraph2.h (see this file to learn further about the goals 
 * of such class).
 */

#include "SubnetGraph2.h"

SubnetGraph2::SubnetGraph2(ERGraph *toProject1, RSGraph *toProject2) : SimpleGraph()
{
    // Makes a copy-construction of each subnet listed in the bipartite graph (no need for sorting)
    list<Vertice*> *subnets = toProject2->getPartyTwo();
    for(list<Vertice*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        Subnet *curSubnet = (Subnet*) (*it);
        Subnet *copy = new Subnet(curSubnet);
        this->vertices.push_back(copy);
        this->copiedSubnets.insert(pair<string, Subnet*>(curSubnet->toString(), copy));
    }
    
    // First looks at the switches and the interconnected routers
    list<Vertice*> *switches = toProject1->getPartyOne();
    for(list<Vertice*>::iterator it = switches->begin(); it != switches->end(); ++it)
    {
        L2Device *curL2 = (L2Device*) (*it);
        
        // Get the routers connected to this switch
        list<Edge*> *incidentEdges = curL2->getIncidentEdges();
        list<Vertice*> routers;
        for(list<Edge*>::iterator i = incidentEdges->begin(); i != incidentEdges->end(); ++i)
            routers.push_back((*i)->getVerticeTwo()); // Router is always the second vertice here
        
        // Get their equivalent in RS graph
        list<Vertice*> *routersRS = toProject2->getPartyOne();
        list<Vertice*> routersBis;
        for(list<Vertice*>::iterator i = routers.begin(); i != routers.end(); ++i)
        {
            L3Device *original = (L3Device*) (*i);
            for(list<Vertice*>::iterator j = routersRS->begin(); j != routersRS->end(); ++j)
            {
                L3Device *eq = (L3Device*) (*j);
                if(eq->toString() == original->toString())
                    routersBis.push_back(eq);
            }
        }
        
        // Gathers the incident edges to all these routers in the RS graph
        list<Edge*> groupedEdges;
        for(list<Vertice*>::iterator i = routersBis.begin(); i != routersBis.end(); ++i)
        {
            L3Device *curL3 = (L3Device*) (*i);
            list<Edge*> *incidentEdges = curL3->getIncidentEdges();
            for(list<Edge*>::iterator j = incidentEdges->begin(); j != incidentEdges->end(); ++j)
                groupedEdges.push_back((*j));
        }
        
        // Finally creates the subnet - subnet edges
        for(list<Edge*>::iterator i = groupedEdges.begin(); i != groupedEdges.end(); ++i)
        {
            // Subnet is always second vertice
            Subnet *subnet1 = this->getEquivalent((Subnet*) (*i)->getVerticeTwo());
            if(subnet1 == NULL)
                continue;

            // Starting at i + 1 ensures no duplicate edge
            for(list<Edge*>::iterator j = ++i; j != groupedEdges.end(); ++j)
            {
                Subnet *subnet2 = this->getEquivalent((Subnet*) (*j)->getVerticeTwo());
                if(subnet2 == NULL)
                    continue;
                
                Graph::createEdge(subnet1, subnet2);
            }
            i--; // To "fix" next iteration
        }
    }
    
    // Now gets the routers that are not connected to L2
    list<Vertice*> *routers = toProject1->getPartyTwo();
    list<Vertice*> *routersBis = toProject2->getPartyOne();
    list<Vertice*>::iterator itBis = routersBis->begin();
    list<Vertice*> independentL3;
    for(list<Vertice*>::iterator it = routers->begin(); it != routers->end(); ++it)
    {
        if((*it)->getIncidentEdges()->size() == 0)
            independentL3.push_back((*itBis));
        itBis++;
    }

    // Creates new edges on the basis of the "independent" routers
    for(list<Vertice*>::iterator it = independentL3.begin(); it != independentL3.end(); ++it)
    {
        L3Device *curL3 = (L3Device*) (*it);

        list<Edge*> *incidentEdges = curL3->getIncidentEdges();
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
    
    // Sort all the edges
    Graph::sortEdges();
}

SubnetGraph2::~SubnetGraph2()
{
    // Nothing, the different items will be deleted by parent constructors
}

Subnet* SubnetGraph2::getEquivalent(Subnet *original)
{
    map<string, Subnet*>::iterator res = copiedSubnets.find(original->toString());
    if(res != copiedSubnets.end())
        return res->second;
    return NULL;
}
