/*
 * IPTableEntry.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in IPTableEntry.h (see this file to learn further about the goals 
 * of such class).
 */

#include "IPTableEntry.h"

IPTableEntry::IPTableEntry(InetAddress ip, unsigned short nbIPIDs) : InetAddress(ip)
{
    // Default values for main fields
    this->TTL = NO_KNOWN_TTL;
    this->preferredTimeout = TimeVal(DEFAULT_TIMEOUT_SECONDS, TimeVal::HALF_A_SECOND);
    
    if(nbIPIDs > MIN_ALIAS_RESOLUTION_PAIRS)
        this->nbIPIDs = nbIPIDs;
    else
        this->nbIPIDs = MIN_ALIAS_RESOLUTION_PAIRS;
    
    // Creates arrays for alias resolution hints
    this->probeTokens = new unsigned long[this->nbIPIDs];
    this->IPIdentifiers = new unsigned short[this->nbIPIDs];
    this->echoMask = new bool[this->nbIPIDs];
    this->delays = new unsigned long[this->nbIPIDs - 1];
    
    // Resets them with 0, "false", etc.
    this->resetARHints();
}

IPTableEntry::~IPTableEntry()
{
    delete[] probeTokens;
    delete[] IPIdentifiers;
    delete[] echoMask;
    delete[] delays;
}

void IPTableEntry::resetARHints()
{
    this->hostName = "";
    for(unsigned short i = 0; i < this->nbIPIDs; i++)
    {
        this->probeTokens[i] = 0;
        this->IPIdentifiers[i] = 0;
        this->echoMask[i] = false;
    }
    for(unsigned short i = 0; i < this->nbIPIDs - 1; i++)
        this->delays[i] = 0;
    this->processedForAR = false;
    this->velocityLowerBound = 0.0;
    this->velocityUpperBound = 0.0;
    this->IPIDCounterType = NO_IDEA;
    this->echoInitialTTL = 0;
    this->replyingToTSRequest = false;
    this->portUnreachableSrcIP = InetAddress(0);
}

bool IPTableEntry::hasHopCount(unsigned char hopCount)
{
    for(list<unsigned char>::iterator it = hopCounts.begin(); it != hopCounts.end(); it++)
    {
        if((*it) == hopCount)
            return true;
    }
    return false;
}

void IPTableEntry::recordHopCount(unsigned char hopCount)
{
    if(hopCount != TTL)
    {
        if(hopCount < TTL)
        {
            if(hopCounts.size() == 0)
                hopCounts.push_back(TTL);
            hopCounts.push_front(hopCount);
            TTL = hopCount;
        }
        else
        {
            if(hopCounts.size() == 0)
                hopCounts.push_back(TTL);
            hopCounts.push_back(hopCount);
        }
    }
}

bool IPTableEntry::hasPreAlias(InetAddress IP)
{
    for(list<InetAddress>::iterator it = preAliases.begin(); it != preAliases.end(); it++)
    {
        if((*it) == IP)
            return true;
    }
    return false;
}

void IPTableEntry::recordPreAlias(InetAddress IP)
{
    for(list<InetAddress>::iterator it = preAliases.begin(); it != preAliases.end(); it++)
    {
        if((*it) == IP)
            return;
    }
    preAliases.push_back(IP);
    preAliases.sort(InetAddress::smaller);
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

bool IPTableEntry::safeIPIDData()
{
    unsigned short previousID = this->IPIdentifiers[0];
    for(unsigned short i = 1; i < nbIPIDs; i++)
    {
        unsigned short curID = this->IPIdentifiers[i];
        if(curID == previousID)
            return false;
        else
            previousID = curID;
    }
    return true;
}

bool IPTableEntry::hasDNS()
{
    if(!hostName.empty())
        return true;
    return false;
}

string IPTableEntry::toString()
{
    stringstream ss;
    
    // IP - TTL
    ss << (*this) << " - " << (unsigned short) TTL;
    
    // : [Initial echo TTL] - ECHO
    if(IPIDCounterType == ECHO_COUNTER)
    {
        ss << ": ";
        
        unsigned short iTTL = (unsigned short) this->echoInitialTTL;
        if(iTTL > 0)
            ss << iTTL << " - ";
        
        ss << "ECHO";
        
        // ,[Host name]
        if(!hostName.empty())
            ss << "," << hostName;
    }
    // : [Initial echo TTL] - [IP-ID data]
    else if(this->hasIPIDData())
    {
        ss << ": ";
        
        unsigned short iTTL = (unsigned short) echoInitialTTL;
        if(iTTL > 0)
            ss << iTTL << " - ";
        
        for(unsigned short i = 0; i < nbIPIDs; i++)
        {
            if(i > 0)
                ss << "," << delays[i - 1] << ",";
            ss << probeTokens[i] << ";" << IPIdentifiers[i];
        }
        
        // ,[Host name]
        if(!hostName.empty())
            ss << "," << hostName;
    }
    // : [Host name]
    else if(!hostName.empty())
    {
        ss << ": " << hostName;
    }
    
    // ... | [Yes or nothing] (yes ~= replies to ICMP timestamp request)
    if(replyingToTSRequest)
    {
        ss << " | Yes";
        
        // ,[Unreachable port reply IP]
        if(portUnreachableSrcIP != InetAddress(0))
            ss << "," << portUnreachableSrcIP;
    }
    // ... | [Unreachable port reply IP] (if available)
    else if(portUnreachableSrcIP != InetAddress(0))
        ss << " | " << portUnreachableSrcIP;
    
    // ... || [pre-aliases, IPs separated by ,]
    if(preAliases.size() > 0)
    {
        ss << " || ";
        for(list<InetAddress>::iterator it = preAliases.begin(); it != preAliases.end(); ++it)
        {
            if(it != preAliases.begin())
                ss << ",";
            ss << (*it);
        }
    }
    
    return ss.str();
}

string IPTableEntry::toStringFingerprint()
{
    stringstream ss;
    
    /*
     * N.B.: here, the Fingerprint class is pretty much useless, as all the data we need is 
     * normally already in this class; except if it has not been processed yet. But this method is 
     * usually not called before processing the data.
     */
    
    ss << (*this) << " - <";
    if(this->echoInitialTTL > 0)
        ss << (unsigned short) this->echoInitialTTL;
    else
        ss << "*";
    ss << ",";
    if(this->portUnreachableSrcIP != InetAddress(0))
        ss << this->portUnreachableSrcIP;
    else
        ss << "*";
    ss << ",";
    switch(this->IPIDCounterType)
    {
        case HEALTHY_COUNTER:
            ss << "Healthy";
            break;
        case RANDOM_COUNTER:
            ss << "Random";
            break;
        case ECHO_COUNTER:
            ss << "Echo";
            break;
        default:
            ss << "*";
            break;
    }
    ss << ",";
    if(!this->hostName.empty())
        ss << "Yes";
    else
        ss << "No";
    ss << ",";
    if(this->replyingToTSRequest)
        ss << "Yes";
    else
        ss << "No";
    ss << ">";

    return ss.str();
}
