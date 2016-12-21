/*
 * Router.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in Router.h (see this file to learn further about the goals of 
 * such class).
 */

#include <string>
using std::string;

#include "Router.h"

using namespace std;

Router::Router()
{
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

IPTableEntry* Router::getMergingPivot(IPLookUpTable *table)
{
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        RouterInterface *interface = (*it);
        if(interface->aliasMethod == RouterInterface::UDP_PORT_UNREACHABLE)
        {
            IPTableEntry *entry = table->lookUp(interface->ip);
            if(entry != NULL && entry->getIPIDCounterType() == IPTableEntry::HEALTHY_COUNTER)
                return entry;
        }
    }
    return NULL;
}

string Router::toString()
{
    stringstream result;
    bool first = true;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(first)
            first = false;
        else
            result << " ";
        result << (*it)->ip;
    }
    return result.str();
}

string Router::toStringVerbose()
{
    stringstream result;
    bool first = true;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(first)
            first = false;
        else
            result << ", ";
        result << (*it)->ip;
        if((*it)->aliasMethod != RouterInterface::FIRST_IP)
        {
            result << " (";
            switch((*it)->aliasMethod)
            {
                case RouterInterface::UDP_PORT_UNREACHABLE:
                    result << "UDP unreachable port";
                    break;
                case RouterInterface::ALLY:
                    result << "Ally";
                    break;
                case RouterInterface::IPID_VELOCITY:
                    result << "IP-ID Velocity";
                    break;
                case RouterInterface::REVERSE_DNS:
                    result << "Reverse DNS";
                    break;
                case RouterInterface::GROUP_ECHO:
                    result << "Echo group";
                    break;
                case RouterInterface::GROUP_ECHO_DNS:
                    result << "Echo group & DNS";
                    break;
                case RouterInterface::GROUP_RANDOM:
                    result << "Random group";
                    break;
                case RouterInterface::GROUP_RANDOM_DNS:
                    result << "Random group & DNS";
                    break;
                default:
                    break;
            }
            result << ")";
        }
    }
    return result.str();
}
