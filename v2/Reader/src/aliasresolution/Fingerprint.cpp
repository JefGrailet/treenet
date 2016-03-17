/*
 * Fingerprint.cpp
 *
 *  Created on: Mar 7, 2016
 *      Author: grailet
 *
 * Implements the class defined in Fingerprint.h (see this file to learn further about its goals).
 */

#include "Fingerprint.h"

Fingerprint::Fingerprint(IPTableEntry *ip)
{
    this->ipEntry = ip;
    
    this->initialTTL = ip->getEchoInitialTTL();
    this->IPIDCounterType = ip->getIPIDCounterType();
    this->hostName = ip->getHostName();
}

Fingerprint::~Fingerprint()
{
}

bool Fingerprint::compare(Fingerprint &f1, Fingerprint &f2)
{
    if (f1.initialTTL > f2.initialTTL)
    {
        return true;
    }
    else if(f1.initialTTL == f2.initialTTL)
    {
        if(f1.IPIDCounterType > f2.IPIDCounterType)
        {
            return true;
        }
        else if(f1.IPIDCounterType == f2.IPIDCounterType)
        {
            if(!f1.hostName.empty() && f2.hostName.empty())
            {
                return true;
            }
        }
    }
    return false;
}

bool Fingerprint::equals(Fingerprint &f)
{
    if(this->initialTTL == f.initialTTL && this->IPIDCounterType == f.IPIDCounterType)
        return true;
    return false;
}

bool Fingerprint::toGroupByDefault()
{
    switch(this->IPIDCounterType)
    {
        case IPTableEntry::RANDOM_COUNTER:
        case IPTableEntry::ECHO_COUNTER:
            return true;
        default:
            break;
    }
    return false;
}
