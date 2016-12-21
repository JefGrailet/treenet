/*
 * Vertice.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Vertice.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Vertice.h"

Vertice::Vertice(unsigned int ID)
{
    this->ID = ID;
}

Vertice::~Vertice()
{
}

Vertice::Vertice(Vertice *toCopy)
{
    ID = toCopy->getID();
    
    // Incident edges are not copied on purpose.
}

bool Vertice::isConnectedTo(Vertice *v)
{
    for(list<Edge*>::iterator it = incidentEdges.begin(); it != incidentEdges.end(); ++it)
    {
        Edge *cur = (*it);
        if(cur->connects(v))
            return true;
    }
    return false;
}
