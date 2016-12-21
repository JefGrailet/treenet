/*
 * Edge.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Edge.h (see this file to learn further about the goals of such 
 * class).
 */

#include "Edge.h"

Edge::Edge(Vertice *s1, Vertice *s2)
{
    string s1Str = s1->toString();
    string s2Str = s2->toString();
    
    if(s1Str.at(0) > s2Str.at(0))
    {
        v1 = s2;
        v2 = s1;
    }
    else
    {
        v1 = s1;
        v2 = s2;
    }
}

Edge::~Edge()
{
    // Connected vertices should be deleted elsewhere
}

bool Edge::connects(Vertice *v)
{
    if(v1 == v || v2 == v)
        return true;
    return false;
}

string Edge::toString()
{
    return v1->toString() + " - " + v2->toString();
}

bool Edge::smaller(Edge *e1, Edge *e2)
{
    Vertice *e1v1 = e1->getVerticeOne();
    Vertice *e2v1 = e2->getVerticeOne();
    
    if(e1v1->getID() > e2v1->getID())
        return false;
    else if(e1v1->getID() == e2v1->getID())
    {
        Vertice *e1v2 = e1->getVerticeTwo();
        Vertice *e2v2 = e2->getVerticeTwo();
        
        if(e1v2->getID() > e2v2->getID())
            return false;
        return true;
    }
    return true;
}
