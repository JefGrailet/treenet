/*
 * ReverseDNSUnit.cpp
 *
 *  Created on: Mar 4, 2015
 *      Author: grailet
 *
 * Implements the class defined in ReverseDNSUnit.h (see this file to learn further about the 
 * goals of such class).
 */

#include "ReverseDNSUnit.h"

ReverseDNSUnit::ReverseDNSUnit(TreeNETEnvironment *e, InetAddress &IP):
env(e), 
IPToProbe(IP)
{
}

ReverseDNSUnit::~ReverseDNSUnit()
{
}

void ReverseDNSUnit::run()
{
    // Gets entry in IP table
    InetAddress target = this->IPToProbe;
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *entry = table->lookUp(target);
    
    // Just in case (normally, should not occur, but we never know with PlanetLab)
    if(entry == NULL)
        return;

    // Gets host name
    string hostName = *(target.getHostName());
    if(!hostName.empty())
    {
        entry->setHostName(hostName);
    }
}
