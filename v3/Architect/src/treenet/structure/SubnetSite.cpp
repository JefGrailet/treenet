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

SubnetSite::SubnetSite():
inferredSubnetBaseIP(0), 
inferredSubnetPrefix(255), 
status(0), 
contrapivot(0), 
TTL1(0), 
TTL2(0), 
routeSize(0), 
processedRouteSize(0), 
route(NULL), 
processedRoute(NULL)
{

}

SubnetSite::~SubnetSite()
{
    clearIPlist();
    if(route != NULL)
        delete[] route;
    if(processedRoute != NULL)
        delete[] processedRoute;
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
    unsigned char prefixLength1 = ss1->getInferredSubnetPrefixLength();
    unsigned long int prefix1 = (ss1->getPivot()).getULongAddress();
    prefix1 = prefix1 >> (32 - prefixLength1);
    prefix1 = prefix1 << (32 - prefixLength1);
    
    unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
    unsigned long int prefix2 = (ss2->getPivot()).getULongAddress();
    prefix2 = prefix2 >> (32 - prefixLength2);
    prefix2 = prefix2 << (32 - prefixLength2);
    
    bool result = false;
    if (prefix1 < prefix2)
        result = true;
    return result;
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

    // No more condition "shortestTTL == greatestTTL - 1" because of outliers
    this->contrapivot = contrapivotCandidate;
    
    /*
     * We also re-compute the subnet status (mainly for ACCURATE/ODD distinction).
     */
    
    if(shortestTTL == greatestTTL)
    {
        this->status = SubnetSite::SHADOW_SUBNET;
    }
    // 2 TTLs with a difference of 1: subnet might be accurate
    else if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        unsigned short nbContrapivot = 0;
        InetAddress smallestContrapivot = contrapivotCandidate;
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            if(cur != NULL && cur->TTL == shortestTTL)
            {
                nbContrapivot++;
                if(cur->ip < smallestContrapivot)
                    smallestContrapivot = cur->ip;
            }
        }
        
        if(nbContrapivot == 1)
            this->status = SubnetSite::ACCURATE_SUBNET;
        else
        {
            this->status = SubnetSite::ODD_SUBNET;
            this->contrapivot = smallestContrapivot;
        }
    }
    // Any other case: subnet is odd
    else
    {
        this->status = SubnetSite::ODD_SUBNET;
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
    // hasRouteLabel() should only be used for the final route (so, maybe a post-processed one)
    if(processedRoute != NULL)
    {
        if(TTL > this->processedRouteSize)
            return false;
    
        if(processedRoute[TTL - 1].ip == rl)
            return true;
        return false;
    }

    if(TTL > this->routeSize)
        return false;
    
    if(route[TTL - 1].ip == rl)
        return true;
    return false;
}

InetAddress SubnetSite::getPivot()
{
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
    
    // 0.0.0.0 if no node (unlikely)
    if(IPlist.size() == 0)
        return InetAddress(0);
    
    // Otherwise first IP in the list (better than nothing)
    return IPlist.front()->ip;
}

list<InetAddress> SubnetSite::getPivotAddresses(unsigned short max)
{
    list<InetAddress> result;
    unsigned short counter = 0;
    
    // Shadow subnet: gets the #max first IPs
    if(this->status == SubnetSite::SHADOW_SUBNET)
    {
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            result.push_back((*i)->ip);
            counter++;
            
            if(counter == max)
                break;
        }
        
        return result;
    }
    
    unsigned short shortestTTL = this->TTL1;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((unsigned short) (*i)->TTL == shortestTTL + 1)
        {
            result.push_back((*i)->ip);
            counter++;
            
            if(counter == max)
                break;
        }
    }
    
    return result;
}

unsigned short SubnetSite::countContrapivotAddresses()
{
    if(this->status == SubnetSite::ACCURATE_SUBNET)
        return 1;
    else if(this->status == SubnetSite::SHADOW_SUBNET)
        return 0;

    unsigned short count = 0;
    unsigned short shortestTTL = this->TTL1;
    
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((unsigned short) (*i)->TTL == shortestTTL)
        {
            count++;
        }
    }
    
    return count;
}

list<InetAddress> SubnetSite::getContrapivotAddresses()
{
    list<InetAddress> result;
    unsigned short shortestTTL = this->TTL1;
    
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((unsigned short) (*i)->TTL == shortestTTL)
        {
            result.push_back((*i)->ip);
        }
    }
    
    return result;
}

bool SubnetSite::hasCompleteRoute()
{
    for(unsigned short i = 0; i < this->routeSize; ++i)
        if(this->route[i].ip == InetAddress(0))
            return false;
    return true;
}

bool SubnetSite::hasIncompleteRoute()
{
    for(unsigned short i = 0; i < this->routeSize; ++i)
        if(this->route[i].ip == InetAddress(0))
            return true;
    return false;
}

unsigned short SubnetSite::countMissingHops()
{
    unsigned short res = 0;
    for(unsigned short i = 0; i < this->routeSize; ++i)
        if(this->route[i].ip == InetAddress(0))
            res++;
    return res;
}

RouteInterface* SubnetSite::getFinalRoute(unsigned short *finalRouteSize)
{
    if(processedRouteSize > 0 && processedRoute != NULL)
    {
        (*finalRouteSize) = processedRouteSize;
        return processedRoute;
    }
    if(routeSize > 0 && route != NULL)
    {
        (*finalRouteSize) = routeSize;
        return route;
    }
    (*finalRouteSize) = 0;
    return NULL;
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
            // Security (for PlanetLab version)
            if((*i) == NULL)
                continue;
        
            // Avoids possible duplicates
            if((*i)->ip == previous)
                continue;

            if(guardian)
                ss << ", ";
            else
                guardian = true;
        
            ss << (*i)->ip << " - " << (unsigned short) (*i)->TTL;

            previous = (*i)->ip;
        }
        ss << "\n";
        
        // Writes (observed) route
        if(this->routeSize > 0 && this->route != NULL)
        {
            guardian = false;
            for(unsigned int i = 0; i < this->routeSize; i++)
            {
                if(guardian)
                    ss << ", ";
                else
                    guardian = true;
                
                unsigned short curState = this->route[i].state;
                if(this->route[i].ip != InetAddress(0))
                {
                    ss << this->route[i].ip;
                    if(curState == RouteInterface::REPAIRED_1)
                        ss << " [Repaired-1]";
                    else if(curState == RouteInterface::REPAIRED_2)
                        ss << " [Repaired-2]";
                    else if(curState == RouteInterface::LIMITED)
                        ss << " [Limited]";
                    else if(curState == RouteInterface::STRETCHED)
                        ss << " [Stretched]";
                    else if(curState == RouteInterface::CYCLE)
                        ss << " [Cycle]";
                    else if(curState == RouteInterface::PREDICTED)
                        ss << " [Predicted]";
                }
                else
                {
                    if(curState == RouteInterface::ANONYMOUS)
                        ss << "Anonymous";
                    else if(curState == RouteInterface::MISSING)
                        ss << "Missing";
                    else
                        ss << "Skipped";
                }
            }
            ss << "\n";
        }
        else
        {
            ss << "No route\n";
        }
        
        // Writes post-processed route if existing (otherwise, regular display)
        if(processedRouteSize > 0 && processedRoute != NULL)
        {
            guardian = false;
            ss << "Post-processed: ";
            for(unsigned int i = 0; i < processedRouteSize; i++)
            {
                if(guardian)
                    ss << ", ";
                else
                    guardian = true;
                
                if(processedRoute[i].ip != InetAddress(0))
                    ss << processedRoute[i].ip;
                else
                    ss << "Anonymous";
            }
            ss << "\n";
        }
    }
    
    return ss.str();
}

bool SubnetSite::isCredible()
{
    bool credibility = false;
    
    unsigned short baseTTL = (unsigned short) this->TTL1;
    unsigned short diffTTL = (unsigned short) this->TTL2 - baseTTL;
    
    if(diffTTL == 0)
        return false;
    
    unsigned int *interfacesByTTL = new unsigned int[diffTTL + 1];
    for (unsigned short i = 0; i < (diffTTL + 1); i++)
        interfacesByTTL[i] = 0;
    
    unsigned int totalInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        // Just in case
        if((*i) == NULL)
            continue;
    
        unsigned short offset = (unsigned short) (*i)->TTL - baseTTL;
        if(offset <= diffTTL)
        {
            interfacesByTTL[offset]++;
            totalInterfaces++;
        }
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
