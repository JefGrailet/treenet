/*
 * Router.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * Implements the class defined in Router.h (see this file to learn further about the goals of 
 * such class).
 */
 
#include <sstream>
using std::stringstream;
#include "Router.h"

Router::Router()
{
    this->bipRouter = NULL;
}

Router::~Router()
{
}

void Router::addInterface(InetAddress interface)
{
    interfacesList.push_back(interface);
    interfacesList.sort(InetAddress::smaller);
}

unsigned short Router::getNbInterfaces()
{
    return (unsigned short) interfacesList.size();
}

bool Router::hasInterface(InetAddress interface)
{
    for(list<InetAddress>::iterator i = interfacesList.begin(); i != interfacesList.end(); ++i)
    {
        if((*i) == interface)
        {
            return true;
        }
    }
    return false;
}

bool Router::givesAccessTo(SubnetSite *ss)
{
    // First find the contra-pivot of the subnet (as a list, as several candidates can occur)
    unsigned char TTL1 = ss->getShortestTTL();
    unsigned char TTL2 = ss->getGreatestTTL();
    if(TTL1 == TTL2)
        return false;
    
    list<InetAddress> candidates;
    list<SubnetSiteNode*> *IPs = ss->getSubnetIPList();
    for(list<SubnetSiteNode*>::iterator i = IPs->begin(); i != IPs->end(); ++i)
    {
        if((*i)->TTL == TTL1)
            candidates.push_back((*i)->ip);
    }
    
    // ss is accessed through this router if the contra-pivot appears in the list of interfaces
    for(list<InetAddress>::iterator i = candidates.begin(); i != candidates.end(); ++i)
    {
        for(list<InetAddress>::iterator j = interfacesList.begin(); j != interfacesList.end(); ++j)
        {
            if((*i) == (*j))
            {
                return true;
            }
        }
    }
    return false;
}

string Router::toString()
{
    stringstream routerStream;
    routerStream << "[";
    bool first = true;
    for(list<InetAddress>::iterator i = interfacesList.begin(); i != interfacesList.end(); ++i)
    {
        if(!first)
            routerStream << ", ";
        else
            first = false;
        routerStream << (*i);
    }
    routerStream << "]";
    
    return routerStream.str();
}

