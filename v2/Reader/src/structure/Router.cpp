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
    for(list<RouterInterface*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        delete (*i);
    }
}

void Router::addInterface(InetAddress interface, unsigned short aliasMethod)
{
    RouterInterface *newInterface = new RouterInterface(interface, aliasMethod);
    interfaces.push_back(newInterface);
    interfaces.sort(RouterInterface::smaller);
}

unsigned short Router::getNbInterfaces()
{
    return (unsigned short) interfaces.size();
}

bool Router::hasInterface(InetAddress interface)
{
    for(list<RouterInterface*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if((*i)->ip == interface)
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
    
    /*
     * Simply checks that some IP of the router does not simply belong to the whole subnet. This 
     * can occur due to routing irregularities or specific network policies.
     */
    
    for(list<RouterInterface*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if(ss->contains((*i)->ip))
        {
            return true;
        }
    }
    
    /*
     * Old method, which consisted in listing Contra-Pivot interfaces and verifying that one of 
     * them is among the interfaces list of the router. Should be outdated now, due to the few 
     * lines above, which not only run faster but also encompass cases where a label of a node 
     * is among a subnet but not a Contra-Pivot in it (can occur with ODD subnets).
    
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
        for(list<RouterInterface*>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
        {
            if((*i) == (*j)->ip)
            {
                return true;
            }
        }
    }
    */
    
    return false;
}

string Router::toString()
{
    stringstream routerStream;
    
    routerStream << "[";
    bool first = true;
    for(list<RouterInterface*>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
    {
        if(!first)
            routerStream << ", ";
        else
            first = false;
        routerStream << (*j)->ip;
        
        // Precise alias resolution method
        switch((*j)->aliasMethod)
        {
            case RouterInterface::ALLY:
                routerStream << " (Ally)";
                break;
            case RouterInterface::IPID_VELOCITY:
                routerStream << " (Velocity)";
                break;
            case RouterInterface::REVERSE_DNS:
                routerStream << " (Reverse DNS)";
                break;
            case RouterInterface::GROUP_ECHO:
                routerStream << " (Echo group)";
                break;
            case RouterInterface::GROUP_ECHO_DNS:
                routerStream << " (Echo group & DNS)";
                break;
            case RouterInterface::GROUP_RANDOM:
                routerStream << " (Random group)";
                break;
            case RouterInterface::GROUP_RANDOM_DNS:
                routerStream << " (Random group & DNS)";
                break;
            default:
                break;
        }
    }
    routerStream << "]";
    
    return routerStream.str();
}

string Router::toStringBis()
{
    stringstream routerStream;
    
    bool first = true;
    for(list<RouterInterface*>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
    {
        if(!first)
            routerStream << " ";
        else
            first = false;
        routerStream << (*j)->ip;
    }
    
    return routerStream.str();
}
