/*
 * Neighborhood.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Neighborhood.h (see this file to learn further about the goals 
 * of such class).
 */

#include <sstream>
using std::stringstream;

#include "Neighborhood.h"

const string Neighborhood::STRING_PREFIX = "N";

Neighborhood::Neighborhood(unsigned int ID, NetworkTreeNode *equivalent) : Vertice(ID)
{
    this->equivalent = equivalent;
}

Neighborhood::~Neighborhood()
{
}

Neighborhood::Neighborhood(Neighborhood *toCopy) : Vertice(toCopy)
{
    equivalent = toCopy->getEquivalent();
}

string Neighborhood::toString()
{
    stringstream ss;
    ss << STRING_PREFIX << ID;
    return ss.str();
}

string Neighborhood::toStringDetailed()
{
    stringstream ss;
    list<InetAddress> *labels = equivalent->getLabels();
    bool first = true;
    
    ss << STRING_PREFIX << ID << " - {";
    for(list<InetAddress>::iterator it = labels->begin(); it != labels->end(); ++it)
    {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << (*it);
    }
    ss << "}";
    
    return ss.str();
}
