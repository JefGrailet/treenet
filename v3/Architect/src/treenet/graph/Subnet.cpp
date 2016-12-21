/*
 * Subnet.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Subnet.h (see this file to learn further about the goals of 
 * such class).
 */

#include <sstream>
using std::stringstream;

#include "Subnet.h"

const string Subnet::STRING_PREFIX = "S";

Subnet::Subnet(unsigned int ID, SubnetSite *equivalent) : Vertice(ID)
{
    type = T_INFERRED;
    this->equivalent = equivalent;
}

Subnet::Subnet(unsigned int ID) : Vertice(ID)
{
    type = T_IMAGINARY;
    equivalent = NULL;
}

Subnet::~Subnet()
{
}

Subnet::Subnet(Subnet *toCopy) : Vertice(toCopy)
{
    type = toCopy->getType();
    equivalent = toCopy->getEquivalent();
}

string Subnet::toString()
{
    stringstream ss;
    ss << STRING_PREFIX << ID;
    return ss.str();
}

string Subnet::toStringDetailed()
{
    stringstream ss;
    ss << STRING_PREFIX << ID << " - ";
    if(type == T_INFERRED)
    {
        if(equivalent != NULL)
            ss << equivalent->getInferredNetworkAddressString();
        else
            ss << "Missing subnet";
    }
    else
    {
        ss << "Imaginary";
    }
    return ss.str();
}
