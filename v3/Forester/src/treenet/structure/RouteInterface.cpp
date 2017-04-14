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

RouteInterface::RouteInterface(InetAddress ip, bool timeout)
{
    this->ip = ip;
    if(ip == InetAddress(0))
    {
        if(timeout)
            this->state = ANONYMOUS;
        else
            this->state = MISSING;
    }
    else
    {
        this->state = VIA_TRACEROUTE;
    }
}

RouteInterface::~RouteInterface()
{
}

void RouteInterface::update(InetAddress ip)
{
    this->ip = ip;
    if(ip == InetAddress(0))
    {
        this->state = ANONYMOUS;
    }
    else
    {
        this->state = VIA_TRACEROUTE;
    }
}

void RouteInterface::anonymize()
{
    this->ip = InetAddress(0);
    this->state = ANONYMOUS;
}

void RouteInterface::repair(InetAddress ip)
{
    this->ip = ip;
    this->state = REPAIRED_1;
}

void RouteInterface::repairBis(InetAddress ip)
{
    this->ip = ip;
    this->state = REPAIRED_2;
}

void RouteInterface::deanonymize(InetAddress ip)
{
    this->ip = ip;
    this->state = LIMITED;
}

void RouteInterface::predict(InetAddress ip)
{
    this->ip = ip;
    this->state = PREDICTED;
}

RouteInterface &RouteInterface::operator=(const RouteInterface &other)
{
    this->ip = other.ip;
    this->state = other.state;
    return *this;
}
