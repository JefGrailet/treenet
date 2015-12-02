/*
 * Router.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * Implements the class defined in Router.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Router.h"

using namespace std;

Router::Router()
{
}

Router::~Router()
{
}

void Router::addInterface(InetAddress interface, unsigned short aliasMethod)
{
    RouterInterface newInterface(interface, aliasMethod);
    interfaces.push_back(newInterface);
    interfaces.sort(RouterInterface::smaller);
}

unsigned short Router::getNbInterfaces()
{
    return (unsigned short) interfaces.size();
}
