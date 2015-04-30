/*
 * ReverseDNSResolverUnit.cpp
 *
 *  Created on: Mar 4, 2015
 *      Author: grailet
 *
 * Implements the class defined in ReverseDNSResolverUnit.h (see this file to learn further about 
 * the goals of such class).
 */

#include "ReverseDNSResolverUnit.h"

ReverseDNSResolverUnit::ReverseDNSResolverUnit(InetAddress *IPToProbe)
{
    this->IPToProbe = IPToProbe;
}

ReverseDNSResolverUnit::~ReverseDNSResolverUnit()
{
}

void ReverseDNSResolverUnit::run()
{
    InetAddress IP(*IPToProbe); // Copy-construction
    
    // Gets host name
    string hostName = *(IP.getHostName());
    if(!hostName.empty())
    {
        IPToProbe->setStoredHostName(hostName);
    }
}

