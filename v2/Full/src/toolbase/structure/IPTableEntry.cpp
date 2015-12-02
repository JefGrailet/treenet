/*
 * IPTableEntry.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: grailet
 *
 * Implements the class defined in IPTableEntry.h (see this file to learn further about the goals 
 * of such class).
 */

#include "IPTableEntry.h"

IPTableEntry::IPTableEntry(InetAddress ip, unsigned short nbIPIDs) : InetAddress(ip)
{
    // Default values
    this->TTL = NO_KNOWN_TTL;
    this->preferredTimeout = TimeVal(DEFAULT_TIMEOUT_SECONDS, TimeVal::HALF_A_SECOND);
    this->storedHostName = "";
    
    if(nbIPIDs > MIN_ALIAS_RESOLUTION_PAIRS)
        this->nbIPIDs = nbIPIDs;
    else
        this->nbIPIDs = MIN_ALIAS_RESOLUTION_PAIRS;
    
    this->probeTokens = new unsigned long[this->nbIPIDs];
    this->IPIdentifiers = new unsigned short[this->nbIPIDs];
    this->delays = new unsigned long[this->nbIPIDs - 1];
    for(unsigned short i = 0; i < this->nbIPIDs; i++)
    {
        this->probeTokens[i] = 0;
        this->IPIdentifiers[i] = 0;
    }
    for(unsigned short i = 0; i < this->nbIPIDs - 1; i++)
        this->delays[i] = 0;
    this->velocityLowerBound = 0.0;
    this->velocityUpperBound = 0.0;
}

IPTableEntry::~IPTableEntry()
{
    delete[] probeTokens;
    delete[] IPIdentifiers;
    delete[] delays;
}

bool IPTableEntry::compare(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1->getULongAddress() < ip2->getULongAddress())
        return true;
    return false;
}

bool IPTableEntry::hasIPIDData()
{
    for(unsigned short i = 0; i < nbIPIDs; i++)
    {
        if(this->probeTokens[i] == 0 || this->IPIdentifiers[i] == 0)
            return false;
    }
    
    for(unsigned short i = 0; i < nbIPIDs - 1; i++)
    {
        if(this->delays[i] == 0)
            return false;
    }
    
    return true;
}

string IPTableEntry::toString()
{
    stringstream ss;
    
    ss << (*this) << " - " << (unsigned short) TTL;
    if(this->hasIPIDData())
    {
        ss << ": ";
        bool first = true;
        for(unsigned short i = 0; i < nbIPIDs; i++)
        {
            if(first)
                first = false;
            else
                ss << "," << this->delays[i - 1] << ",";
            ss << this->probeTokens[i] << ";" << this->IPIdentifiers[i];
        }
        if(!storedHostName.empty())
            ss << "," << storedHostName;
    }
    else if(!storedHostName.empty())
    {
        ss << ": " << storedHostName;
    }
    
    return ss.str();
}
