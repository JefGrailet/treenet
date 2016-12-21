/*
 * RouteInterface.cpp
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in RouteInterface.h (see this file to learn further about the 
 * goals of such class).
 */

#include "RouteInterface.h"

RouteInterface::RouteInterface()
{
    this->ip = InetAddress(0);
    this->state = NOT_MEASURED;
}

RouteInterface::RouteInterface(InetAddress ip)
{
    this->ip = ip;
    if(ip == InetAddress("0.0.0.0"))
    {
        this->state = MISSING;
    }
    else
    {
        this->state = VIA_TRACEROUTE;
    }
}

void RouteInterface::update(InetAddress ip)
{
    this->ip = ip;
    if(ip == InetAddress("0.0.0.0"))
    {
        this->state = MISSING;
    }
    else
    {
        this->state = VIA_TRACEROUTE;
    }
}

RouteInterface &RouteInterface::operator=(const RouteInterface &other)
{
    this->ip = other.ip;
    this->state = other.state;
    return *this;
}

void RouteInterface::repair(InetAddress ip)
{
    this->ip = ip;
    this->state = REPAIRED;
}

RouteInterface::~RouteInterface()
{
}
