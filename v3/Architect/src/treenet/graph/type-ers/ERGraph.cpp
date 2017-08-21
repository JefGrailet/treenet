/*
 * ERGraph.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in ERGraph.h (see this file to learn further about the goals of 
 * such class).
 */

#include "ERGraph.h"

ERGraph::ERGraph() : BipartiteGraph()
{
}

ERGraph::~ERGraph()
{
    // Nothing, the items in the maps will be deleted by parent constructors
}

L2Device* ERGraph::createSwitch()
{
    L2Device *l2 = new L2Device(this->counterPartyOne);
    this->counterPartyOne++;
    this->partyOne.push_back(l2);
    return l2;
}

L3Device* ERGraph::lookUp(Router *router)
{
    map<Router*, L3Device*>::iterator res = routers.find(router);
    if(res != routers.end())
        return res->second;
    return NULL;
}

L3Device* ERGraph::lookUpImaginary(NetworkTreeNode *neighborhood)
{
    map<NetworkTreeNode*, L3Device*>::iterator res = imaginaryRouters.find(neighborhood);
    if(res != imaginaryRouters.end())
        return res->second;
    return NULL;
}

void ERGraph::copyRouters(RSGraph *bipRS)
{
    // First resets current routers (if there were already routers)
    if(this->partyTwo.size() > 0)
    {
        for(list<Vertice*>::iterator it = this->partyTwo.begin(); it != this->partyTwo.end(); ++it)
            delete (*it);
        
        this->partyTwo.clear();
        this->counterPartyTwo = 0;
    }

    // Then copies routers of the other graph
    list<Vertice*> *routersRS = bipRS->getPartyOne();    
    for(list<Vertice*>::iterator it = routersRS->begin(); it != routersRS->end(); ++it)
    {
        L3Device *cur = (L3Device*) (*it);
        L3Device *copy = new L3Device((L3Device*) (*it));
        Router *equivalent = copy->getEquivalent();
        
        if(equivalent != NULL)
        {
            routers.insert(pair<Router*, L3Device*>(equivalent, copy));
        }
        else
        {
            NetworkTreeNode *neighborhood = bipRS->getNeighborhoodMatching(cur);
            if(neighborhood != NULL)
            {
                imaginaryRouters.insert(pair<NetworkTreeNode*, L3Device*>(neighborhood, copy));
            }
        }
        this->partyTwo.push_back(copy);
    }
    this->counterPartyTwo = this->partyTwo.size();
}

void ERGraph::connectRouters(list<Router*> routers, NetworkTreeNode *withImaginary)
{
    // Creates virtual switch and connects it to the routers
    L2Device *l2 = createSwitch();
    for(list<Router*>::iterator it = routers.begin(); it != routers.end(); ++it)
    {
        Router *cur = (*it);
        
        L3Device *l3 = lookUp(cur);
        if(l3 == NULL)
            continue; // Should not happen, technically, but just in case
        
        Graph::createEdge(l2, l3);
    }
    
    if(withImaginary != NULL)
    {
        L3Device *imaginaryRouter = lookUpImaginary(withImaginary);
        
        /*
         * Imaginary router should exist by construction: see in ERSProcesser.cpp how the method 
         * processRecursiveRS() is called before processRecursiveER() and should therefore have 
         * created the imaginary component.
         */
        
        if(imaginaryRouter != NULL)
        {
            Graph::createEdge(l2, imaginaryRouter);
        }
    }
}

void ERGraph::linkAllRouters(NetworkTreeNode *neighborhood)
{
    L3Device *imaginaryRouter = lookUpImaginary(neighborhood);
    list<Router*> routers = neighborhood->getInferredRouters();
    if((routers.size() <= 1 && imaginaryRouter == NULL) || routers.size() == 0)
    {
        return; // Nothing to do, there is only one router
    }

    // Creates virtual switch and connects it to the routers
    L2Device *l2 = createSwitch();
    for(list<Router*>::iterator it = routers.begin(); it != routers.end(); ++it)
    {
        Router *cur = (*it);
        
        L3Device *l3 = lookUp(cur);
        if(l3 == NULL)
            continue; // Should not happen, technically, but just in case
        
        Graph::createEdge(l2, l3);
    }
    
    // Connects switch with imaginary router (if any)
    if(imaginaryRouter != NULL)
    {
        Graph::createEdge(l2, imaginaryRouter);
    }
}
