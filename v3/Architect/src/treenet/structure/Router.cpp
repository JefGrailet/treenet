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
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << " ";
        result << (*it)->ip;
    }
    return result.str();
}

string Router::toStringVerbose()
{
    stringstream result;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
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

string Router::toStringMinimalist()
{
    stringstream result;
    result << "[";
    bool shortened = false;
    unsigned short i = 0;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        
        if(i == 3)
        {
            shortened = true;
            result << "...";
            break;
        }
        else
            result << (*it)->ip;
        i++;
    }
    result << "]";
    if(shortened)
    {
        result << " (" << interfaces.size() << " interfaces)";
    }
    return result.str();
}

bool Router::compare(Router *r1, Router *r2)
{
    unsigned short size1 = r1->getNbInterfaces();
    unsigned short size2 = r2->getNbInterfaces();
    
    bool result = false;
    if (size1 < size2)
    {
        result = true;
    }
    else if(size1 == size2)
    {
        list<RouterInterface*> *list1 = r1->getInterfacesList();
        list<RouterInterface*> *list2 = r2->getInterfacesList();
        
        list<RouterInterface*>::iterator it2 = list2->begin();
        for(list<RouterInterface*>::iterator it1 = list1->begin(); it1 != list1->end(); it1++)
        {
            InetAddress ip1 = (*it1)->ip;
            InetAddress ip2 = (*it2)->ip;
            
            if(ip1 < ip2)
            {
                result = true;
                break;
            }
            else if(ip1 > ip2)
            {
                result = false;
                break;
            }
        
            it2++;
        }
    }
    return result;
}

bool Router::equals(Router *other)
{
    unsigned short size1 = (unsigned short) interfaces.size();
    unsigned short size2 = other->getNbInterfaces();
    
    if(size1 == size2)
    {
        list<RouterInterface*> *list2 = other->getInterfacesList();
        list<RouterInterface*>::iterator it2 = list2->begin();
        for(list<RouterInterface*>::iterator it1 = interfaces.begin(); it1 != interfaces.end(); it1++)
        {
            InetAddress ip1 = (*it1)->ip;
            InetAddress ip2 = (*it2)->ip;
            
            if(ip1 < ip2 || ip1 > ip2)
                return false;
        
            it2++;
        }
        
        return true;
    }
    return false;
}
