/*
 * RouterInterface.cpp
 *
 *  Created on: Oct 28, 2015
 *      Author: grailet
 *
 * Implements the class defined in RouterInterface.h (see this file to learn further about the 
 * goals of such class).
 */

#include "RouterInterface.h"

RouterInterface::RouterInterface(InetAddress ip, unsigned short aliasMethod)
{
    this->ip = ip;
    this->aliasMethod = aliasMethod;
}

RouterInterface::~RouterInterface()
{
}
