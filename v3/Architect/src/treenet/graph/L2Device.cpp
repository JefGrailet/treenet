/*
 * L2Device.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in L2Device.h (see this file to learn further about the goals of 
 * such class).
 */

#include <sstream>
using std::stringstream;

#include "L2Device.h"

const string L2Device::STRING_PREFIX = "E";

L2Device::L2Device(unsigned int ID) : Vertice(ID)
{
    type = T_IMAGINARY;
}

L2Device::~L2Device()
{
}

L2Device::L2Device(L2Device *toCopy) : Vertice(toCopy)
{
    type = toCopy->getType();
}

string L2Device::toString()
{
    stringstream ss;
    ss << STRING_PREFIX << ID;
    return ss.str();
}

string L2Device::toStringDetailed()
{
    stringstream ss;
    ss << STRING_PREFIX << ID << " - Imaginary";
    return ss.str();
}
