/*
 * InferredRouter.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * Implements the class defined in InferredRouter.h (see this file to learn further about the 
 * goals of such class).
 */

#include "InferredRouter.h"

using namespace std;

InferredRouter::InferredRouter()
{
}

InferredRouter::~InferredRouter()
{
}

void InferredRouter::addInterface(InetAddress interface, unsigned short aliasMethod)
{
    RouterInterface newInterface(interface, aliasMethod);
    interfaces.push_back(newInterface);
    interfaces.sort(RouterInterface::smaller);
}

unsigned short InferredRouter::getNbInterfaces()
{
    return (unsigned short) interfaces.size();
}
