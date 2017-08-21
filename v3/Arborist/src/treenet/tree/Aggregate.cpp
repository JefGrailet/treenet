/*
 * Aggregate.cpp
 *
 *  Created on: Aug 21, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Aggregate.h (see this file to learn further about the 
 * goals of such class).
 */

#include "Aggregate.h"

Aggregate::Aggregate(InetAddress lastHop, list<InetAddress> interfaces)
{
    lastHops.push_back(lastHop);
    for(list<InetAddress>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
        candidates.push_back(InetAddress((*it)));
}

Aggregate::~Aggregate()
{
    for(list<Router*>::iterator i = inferredRouters.begin(); i != inferredRouters.end(); ++i)
    {
        delete (*i);
    }
    inferredRouters.clear();
}

list<InetAddress> Aggregate::listAllInterfaces()
{
    list<InetAddress> result;
    for(list<InetAddress>::iterator it = lastHops.begin(); it != lastHops.end(); ++it)
        result.push_back(InetAddress((*it)));
    for(list<InetAddress>::iterator it = candidates.begin(); it != candidates.end(); ++it)
        result.push_back(InetAddress((*it)));
    result.sort(InetAddress::smaller);
    return result;
}

bool Aggregate::compare(Aggregate *a1, Aggregate *a2)
{
    InetAddress ip1 = a1->getFirstLastHop();
    InetAddress ip2 = a2->getFirstLastHop();
    if(ip1 < ip2)
        return true;
    return false;
}
