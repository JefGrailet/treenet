/*
 * SubnetSite.cpp
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Implements the class defined in SubnetSite.h (see this class for more details).
 */

#include <string> // For compare() in a static comparison method
#include <math.h> // For pow() function

#include "SubnetSite.h"

#include "../common/inet/NetworkAddress.h"

SubnetSite::SubnetSite():
inferredSubnetBaseIP(0),
inferredSubnetPrefix(255),
status(0),
contrapivot(0),
TTL1(0),
TTL2(0),
routeSize(0),
route(0),
bipSubnet(NULL)
{

}

SubnetSite::~SubnetSite()
{
	clearIPlist();
	if(route != 0)
	    delete[] route;
}

void SubnetSite::clearIPlist()
{
	for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
	{
		delete (*i);
	}
	IPlist.clear();
}

int SubnetSite::getInferredSubnetSize()
{
	int size = 0;
	for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
	{
		size++;
	}
	return size;
}

string SubnetSite::getInferredNetworkAddressString()
{
	if(inferredSubnetPrefix > 32)
	{
		return string("");
	}
	else if(inferredSubnetPrefix == 32)
	{
		return (*(inferredSubnetBaseIP.getHumanReadableRepresentation())) + "/32";
	}
	else
	{
		NetworkAddress na(inferredSubnetBaseIP, inferredSubnetPrefix);
		return (*(na.getHumanReadableRepresentation()));
	}
}

bool SubnetSite::compare(SubnetSite *ss1, SubnetSite *ss2)
{
    if (ss1->getInferredSubnetBaseIP() < ss2->getInferredSubnetBaseIP())
        return true;
    return false;
}

bool SubnetSite::compareRoutes(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned short size1 = ss1->getRouteSize();
    unsigned short size2 = ss2->getRouteSize();

    if(size1 == 0 && size2 == 0)
        return compare(ss1, ss2);
    
    if (size1 < size2)
        return true;
    return false;
}

void SubnetSite::completeRefinedData()
{
    // Goes through the list of IPs to find shortest/greatest TTL
    unsigned char shortestTTL = 0, greatestTTL = 0;
    InetAddress contrapivotCandidate(0);
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
	{
	    SubnetSiteNode *cur = (*i);

        if(greatestTTL == 0 || cur->TTL > greatestTTL)
            greatestTTL = cur->TTL;
        
        if(shortestTTL == 0 || cur->TTL < shortestTTL)
        {
            contrapivotCandidate = cur->ip;
            shortestTTL = cur->TTL;
        }
	}

    this->TTL1 = shortestTTL;
	this->TTL2 = greatestTTL;
	
	if(shortestTTL == (greatestTTL - 1))
	{
	    this->contrapivot = contrapivotCandidate;
	}
}

void SubnetSite::adaptTTLs(unsigned short pivotTTL)
{
    unsigned short baseTTL = pivotTTL - 1;
    bool shorterTTL = false;
    if(baseTTL == this->TTL1)
        return;
    else if(baseTTL > this->TTL1)
        shorterTTL = true;
    
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
	{
	    SubnetSiteNode *cur = (*i);
	    
	    if(shorterTTL)
            cur->TTL -= (this->TTL1 - baseTTL);
        else
            cur->TTL += (baseTTL - this->TTL1);
	}
}

bool SubnetSite::contains(InetAddress i)
{
    NetworkAddress na = this->getInferredNetworkAddress();
    InetAddress lowerBorder = na.getLowerBorderAddress();
    InetAddress upperBorder = na.getUpperBorderAddress();
    
    if(i >= lowerBorder && i <= upperBorder)
        return true;
    return false;
}

bool SubnetSite::hasLiveInterface(InetAddress li)
{
	for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
	{
		if((*i)->ip == li)
		    return true;
	}
	return false;
}

bool SubnetSite::hasRouteLabel(InetAddress rl, unsigned short TTL)
{
    if(TTL > this->routeSize)
        return false;
    
    if(route[TTL - 1] == rl)
        return true;
    return false;
}

InetAddress SubnetSite::getPivotAddress()
{
    // Undefined: aborts
    if(this->status == SubnetSite::UNDEFINED_SUBNET)
    {
        return InetAddress(0);
    }
    
    // Shadow subnet: gets the IP of the first "visible" address
    if(this->status == SubnetSite::SHADOW_SUBNET)
    {
        return IPlist.front()->ip;
    }
    
    unsigned short shortestTTL = this->TTL1;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((unsigned short) (*i)->TTL == shortestTTL + 1)
        {
            return (*i)->ip;
        }
    }
    
    // Null if no result (very unlikely)
    return InetAddress(0);
}

string SubnetSite::toString()
{
    stringstream ss;

    if((this->status == SubnetSite::ACCURATE_SUBNET ||
        this->status == SubnetSite::SHADOW_SUBNET ||
        this->status == SubnetSite::ODD_SUBNET) && 
        this->route != 0)
    {
        ss << this->getInferredNetworkAddressString() << "\n";
        if(this->status == SubnetSite::ACCURATE_SUBNET)
            ss << "ACCURATE\n";
        else if(this->status == SubnetSite::SHADOW_SUBNET)
            ss << "SHADOW\n";
        else
            ss << "ODD\n";
        
        // Writes live interfaces
        IPlist.sort(SubnetSiteNode::smaller); // Sorts the interfaces
        bool guardian = false;
        InetAddress previous(0);
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            // Avoids possible duplicates
            if((*i)->ip == previous)
                continue;
        
            if(guardian)
                ss << ", ";
            else
                guardian = true;
        
            ss << (*i)->ip << " - " << (unsigned short) (*i)->TTL;
            
            InetAddress cur = (*i)->ip;
            string hostName = cur.getStoredHostName();
            if(cur.hasIPIdentifier())
            {
                ss << " [" << cur.getProbeToken() << "|" << cur.getIPIdentifier();
                if(!hostName.empty())
                    ss << "|" << hostName;
                ss << "]";
            }
            else if(!hostName.empty())
            {
                ss << " [" << hostName << "]";
            }
            
            previous = (*i)->ip;
        }
        ss << "\n";
        
        // Writes route
        guardian = false;
        for(unsigned int i = 0; i < this->routeSize; i++)
        {
            if(guardian)
                ss << ", ";
            else
                guardian = true;
            
            ss << this->route[i];
            
            InetAddress cur = this->route[i];
            string hostName = cur.getStoredHostName();
            if(cur.hasIPIdentifier())
            {
                ss << " [" << cur.getProbeToken() << "|" << cur.getIPIdentifier();
                if(!hostName.empty())
                    ss << "|" << hostName;
                ss << "]";
            }
            else if(!hostName.empty())
            {
                ss << " [" << hostName << "]";
            }
        }
        ss << "\n";
    }
    
    return ss.str();
}

bool SubnetSite::isCredible()
{
    bool credibility = false;
    
    unsigned short baseTTL = this->TTL1;
    unsigned short diffTTL = this->TTL2 - baseTTL;
    
    if(diffTTL == 0)
        return false;
    
    unsigned int *interfacesByTTL = new unsigned int[diffTTL + 1];
    for (unsigned short i = 0; i < (diffTTL + 1); i++)
        interfacesByTTL[i] = 0;
    
    unsigned int totalInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        interfacesByTTL[((unsigned short) (*i)->TTL - baseTTL)]++;
        totalInterfaces++;
    }
    
    if(diffTTL == 1)
    {
        if(interfacesByTTL[0] == 1)
            credibility = true;
        else
        {
            double ratioContrapivots = (double) interfacesByTTL[0] / (double) totalInterfaces;
            double ratioPivots = (double) interfacesByTTL[1] / (double) totalInterfaces;
            
            if(ratioPivots > ratioContrapivots && (ratioPivots - ratioContrapivots) > 0.25)
            {
                credibility = true;
            }
        }
    }
    else
    {
        double ratioContrapivots = (double) interfacesByTTL[0] / (double) totalInterfaces;
        double ratioPivots = (double) interfacesByTTL[1] / (double) totalInterfaces;
        
        if(ratioPivots >= 0.7 && ratioContrapivots < 0.1)
        {
            credibility = true;
        }
    }
    
    delete[] interfacesByTTL;
    
    return credibility;
}

unsigned int SubnetSite::getCapacity()
{
    double power = 32 - (double) ((unsigned short) inferredSubnetPrefix);
    
    if(power < 0)
        return 0;
    else if(power == 0)
        return 1;
    
    return (unsigned int) pow(2, power);
}

