/*
 * RSGraph.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in RSGraph.h (see this file to learn further about the goals of 
 * such class).
 */

#include "RSGraph.h"

RSGraph::RSGraph() : BipartiteGraph()
{
}

RSGraph::~RSGraph()
{
    // Nothing, the items in the maps will be deleted by parent constructors
}

L3Device* RSGraph::insert(NetworkTreeNode *neighborhood)
{
    L3Device *l3 = new L3Device(this->counterPartyOne);
    this->counterPartyOne++;
    this->partyOne.push_back(l3);
    imaginaryRouters.insert(pair<NetworkTreeNode*, L3Device*>(neighborhood, l3));
    neighborhoods.insert(pair<L3Device*, NetworkTreeNode*>(l3, neighborhood));
    return l3;
}

L3Device* RSGraph::insert(Router *router)
{
    L3Device *l3 = new L3Device(this->counterPartyOne, router);
    this->counterPartyOne++;
    this->partyOne.push_back(l3);
    routers.insert(pair<Router*, L3Device*>(router, l3));
    return l3;
}

Subnet* RSGraph::insert(SubnetSite *subnet)
{
    Subnet *s = new Subnet(this->counterPartyTwo, subnet);
    this->counterPartyTwo++;
    this->partyTwo.push_back(s);
    subnets.insert(pair<SubnetSite*, Subnet*>(subnet, s));
    return s;
}

L3Device* RSGraph::lookUp(NetworkTreeNode *neighborhood)
{
    map<NetworkTreeNode*, L3Device*>::iterator res = imaginaryRouters.find(neighborhood);
    if(res != imaginaryRouters.end())
        return res->second;
    return NULL;
}

L3Device* RSGraph::lookUp(Router *router)
{
    map<Router*, L3Device*>::iterator res = routers.find(router);
    if(res != routers.end())
        return res->second;
    return NULL;
}

Subnet* RSGraph::lookUp(SubnetSite *subnet)
{
    map<SubnetSite*, Subnet*>::iterator res = subnets.find(subnet);
    if(res != subnets.end())
        return res->second;
    return NULL;
}

void RSGraph::createEdge(Router *router, SubnetSite *subnet)
{
    // Looks up router, or creates it
    L3Device *l3 = lookUp(router);
    if(l3 == NULL)
        l3 = insert(router);
    
    // Does the same with the subnet
    Subnet *s = lookUp(subnet);
    if(s == NULL)
        s = insert(subnet);
    
    // Calls method of superclass to finish
    Graph::createEdge(l3, s);
}

void RSGraph::createEdge(NetworkTreeNode *neighborhood, SubnetSite *subnet)
{
    // Looks up router, or creates it
    L3Device *l3 = lookUp(neighborhood);
    if(l3 == NULL)
        l3 = insert(neighborhood);
    
    // Does the same with the subnet
    Subnet *s = lookUp(subnet);
    if(s == NULL)
        s = insert(subnet);
    
    // Calls method of superclass to finish
    Graph::createEdge(l3, s);
}

void RSGraph::connectL3Devices(L3Device *l3_1, L3Device *l3_2)
{
    /*
     * Checks if l3_1 is connected to l3_2 via an imaginary subnet. The idea is to go through the 
     * list of subnets and checking each imaginary subnet (to be exhaustive) rather than checking 
     * the edges of each vertice.
     */
    
    bool alreadyConnected = false;
    for(list<Vertice*>::iterator it = this->partyTwo.begin(); it != this->partyTwo.end(); ++it)
    {
        Subnet *cur = (Subnet*) (*it);
        if(cur->isImaginary())
        {
            if(cur->isConnectedTo(l3_1) && cur->isConnectedTo(l3_2))
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
        
        Graph::createEdge(l3_1, imaginarySubnet);
        Graph::createEdge(l3_2, imaginarySubnet);
    }
}

void RSGraph::createArtificialPath(Router *r1, Router *r2)
{
    // Looks up routers, or creates them
    L3Device *l3_1 = lookUp(r1);
    if(l3_1 == NULL)
        l3_1 = insert(r1);
    
    L3Device *l3_2 = lookUp(r2);
    if(l3_2 == NULL)
        l3_2 = insert(r2);
    
    this->connectL3Devices(l3_1, l3_2);
}

void RSGraph::createArtificialPath(Router *r, NetworkTreeNode *n)
{
    // Looks up real router and imaginary router, or creates them
    L3Device *l3_1 = lookUp(r);
    if(l3_1 == NULL)
        l3_1 = insert(r);
    
    L3Device *l3_2 = lookUp(n);
    if(l3_2 == NULL)
        l3_2 = insert(n);
    
    this->connectL3Devices(l3_1, l3_2);
}

void RSGraph::createArtificialPath(NetworkTreeNode *n1, NetworkTreeNode *n2)
{
    // Looks up imaginary routers, or creates them
    L3Device *l3_1 = lookUp(n1);
    if(l3_1 == NULL)
        l3_1 = insert(n1);
    
    L3Device *l3_2 = lookUp(n2);
    if(l3_2 == NULL)
        l3_2 = insert(n2);
    
    this->connectL3Devices(l3_1, l3_2);
}

NetworkTreeNode* RSGraph::getNeighborhoodMatching(L3Device *imaginary)
{
    map<L3Device*, NetworkTreeNode*>::iterator res = neighborhoods.find(imaginary);
    if(res != neighborhoods.end())
        return res->second;
    return NULL;
}
