/*
 * L3Device.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in L3Device.h (see this file to learn further about the goals of 
 * such class).
 */

#include <sstream>
using std::stringstream;

#include "L3Device.h"

const string L3Device::STRING_PREFIX = "R";

L3Device::L3Device(unsigned int ID, Router *equivalent) : Vertice(ID)
{
    type = T_INFERRED;
    this->equivalent = equivalent;
}

L3Device::L3Device(unsigned int ID) : Vertice(ID)
{
    type = T_IMAGINARY;
    equivalent = NULL;
}

L3Device::~L3Device()
{
}

L3Device::L3Device(L3Device *toCopy) : Vertice(toCopy)
{
    type = toCopy->getType();
    equivalent = toCopy->getEquivalent();
}

string L3Device::toString()
{
    stringstream ss;
    ss << STRING_PREFIX << ID;
    return ss.str();
}

string L3Device::toStringDetailed()
{
    stringstream ss;
    ss << STRING_PREFIX << ID << " - ";
    if(type == T_INFERRED)
    {
        if(equivalent != NULL)
            ss << equivalent->toStringVerbose();
        else
            ss << "Missing router";
    }
    else
    {
        ss << "Imaginary";
    }
    return ss.str();
}
