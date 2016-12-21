/*
 * NSGraph.cpp
 *
 *  Created on: Nov 29, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in NSGraph.h (see this file to learn further about the goals of 
 * such class).
 */

#include "NSGraph.h"

NSGraph::NSGraph() : BipartiteGraph()
{
}

NSGraph::~NSGraph()
{
    // Nothing, the items in the maps will be deleted by parent constructors
}

Neighborhood* NSGraph::insert(NetworkTreeNode *neighborhood)
{
    Neighborhood *n = new Neighborhood(this->counterPartyOne, neighborhood);
    this->counterPartyOne++;
    this->partyOne.push_back(n);
    neighborhoods.insert(pair<NetworkTreeNode*, Neighborhood*>(neighborhood, n));
    return n;
}

Subnet* NSGraph::insert(SubnetSite *subnet)
{
    Subnet *s = new Subnet(this->counterPartyTwo, subnet);
    this->counterPartyTwo++;
    this->partyTwo.push_back(s);
    subnets.insert(pair<SubnetSite*, Subnet*>(subnet, s));
    return s;
}

Neighborhood* NSGraph::lookUp(NetworkTreeNode *neighborhood)
{
    map<NetworkTreeNode*, Neighborhood*>::iterator res = neighborhoods.find(neighborhood);
    if(res != neighborhoods.end())
        return res->second;
    return NULL;
}

Subnet* NSGraph::lookUp(SubnetSite *subnet)
{
    map<SubnetSite*, Subnet*>::iterator res = subnets.find(subnet);
    if(res != subnets.end())
        return res->second;
    return NULL;
}

void NSGraph::createEdge(NetworkTreeNode *neighborhood, SubnetSite *subnet)
{
    // Looks up neighborhood, or creates it
    Neighborhood *n = lookUp(neighborhood);
    if(n == NULL)
        n = insert(neighborhood);
    
    // Does the same with the subnet
    Subnet *s = lookUp(subnet);
    if(s == NULL)
        s = insert(subnet);
    
    // Calls method of superclass to finish
    Graph::createEdge(n, s);
}

void NSGraph::createArtificialPath(NetworkTreeNode *neighborhood1, NetworkTreeNode *neighborhood2)
{
    // Looks up first neighborhood, or creates it
    Neighborhood *n1 = lookUp(neighborhood1);
    if(n1 == NULL)
        n1 = insert(neighborhood1);
    
    // Does the same for the second
    Neighborhood *n2 = lookUp(neighborhood2);
    if(n2 == NULL)
        n2 = insert(neighborhood2);
    
    /*
     * Checks if n1 is connected to n2 via an imaginary subnet. The idea is to go through the list 
     * of subnets and checking each imaginary subnet (to be exhaustive) rather than checking the 
     * edges of each vertice.
     */
    
    bool alreadyConnected = false;
    for(list<Vertice*>::iterator it = this->partyTwo.begin(); it != this->partyTwo.end(); ++it)
    {
        Subnet *cur = (Subnet*) (*it);
        
        if(cur->isImaginary())
        {
            if(cur->isConnectedTo(n1) && cur->isConnectedTo(n2))
            {
                alreadyConnected = true;
                break;
            }
        }
    }
    
    if(!alreadyConnected)
    {
        Subnet *imaginarySubnet = new Subnet(this->counterPartyTwo);
        this->counterPartyTwo++;
        this->partyTwo.push_back(imaginarySubnet);
        
        Graph::createEdge(n1, imaginarySubnet);
        Graph::createEdge(n2, imaginarySubnet);
    }
}
