/*
 * SubnetSite.cpp
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Implements the class defined in SubnetSite.h (see this class for more details).
 */

#include <string> // For compare() in a static comparison method

#include "SubnetSite.h"

#include "../../common/inet/NetworkAddress.h"

SubnetSite::SubnetSite():
spCost(0),
siCost(0),
mergeAmount(0),
targetIPaddress(0),
pivotIPaddress(0),
prevSiteIPaddress(0),
prevSiteIPaddressDistance(0),
inferredSubnetPrefix(255),
alternativeSubnetPrefix(255),
contraPivotNode(0),
refinementStatus(0),
refinementContrapivot(0),
refinementTTL1(0),
refinementTTL2(0),
refinementRouteSize(0),
refinementRoute(NULL),
routeRepairMask(NULL)
{

}

SubnetSite::~SubnetSite()
{
    clearIPlist();
    if(refinementRoute != NULL)
        delete[] refinementRoute;
    
    if(routeRepairMask != NULL)
        delete[] routeRepairMask;
}

void SubnetSite::clearIPlist()
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL)
            delete (*i);
    }
    IPlist.clear();
}

void SubnetSite::markSubnetOvergrowthElements(unsigned char barrierPrefix, int filterin)
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < barrierPrefix)
            {
                (*i)->nodeStatus = SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH;
            }
        }
    }
}

void SubnetSite::markSubnetBoundaryIncompatibileElements(unsigned char basePrefix, int filterin)
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < basePrefix)
            {
                (*i)->nodeStatus = SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY;
            }
        }
    }
}

void SubnetSite::adjustRemoteSubnet()
{
    /**
     * Now we have the complete list of subnet elements as well as the aliases (if possible). 
     * Before storing the results to the database we must apply the following rules to preserve 
     * the consistency of the subnet:
     * 1) The lower-border-address and upper-border-address cannot be assigned to a node unless 
     *    it is a /31 subnet
     * 2) If either of upper or lower border address is assigned to a node of subnet /x, x <= 30 
     *    then there are two cases:
     *       i)  The subnet is actually a /x-1 or less subnet but we could not discover it completely;
     *       ii) The subnet is /x+1 or greater subnet but we misconcluded and merged it.
     *
     * As an action, as long as the subnet contains a border address, we divide it into half 
     * until a /31 address.
     */

    /**
     * Normalize the site subnet, that is, remove misinferred items in the subnet and set a 
     * prefixLength for the subnet. Note that the minimum subnetPrefix does not always reflect the 
     * subnetPrefix because if we have a subnet with IPs ending with binary {00,01,10}, this 
     * cannot be a /30 subnet even though the minimum site node is discovered while searching for 
     * /30 subnet. This anomaly raises only if we have /30 as minimum site node prefix and the 
     * reason is different pattern caused by /31.
     */

    /**
     * Moreover, if there are maxSubnetSize IP addresses of subnet has been discovered check if 
     * one has distance d-1 (contra-pivot, i.e., alias) while others have distance d. In case all 
     * of them has distance d than expand the subnet size to cover the contra pivot.
     */

    unsigned char subnetPrefix = this->getMinimumPrefixLength(SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

    // Marks boundary address incompatible nodes
    NetworkAddress tmpSubnet(InetAddress(0),31); // Just to temporarily initialize
    bool hasLowerBorder;
    bool hasUpperBorder;
    while(subnetPrefix < 31)
    {
        tmpSubnet = NetworkAddress(this->pivotIPaddress, subnetPrefix);
        hasLowerBorder = this->contains(tmpSubnet.getLowerBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
        hasUpperBorder = this->contains(tmpSubnet.getUpperBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

        if(hasLowerBorder || hasUpperBorder)
        {
            this->markSubnetBoundaryIncompatibileElements((subnetPrefix + 1), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
            // Marks all subnets having prefix less than subnetPrefix
        }
        else
        {
            break;
        }
        subnetPrefix++;
    }

    /**
     * Sets inferred and alternative prefix lengths. If there is no contrapivot, just use the 
     * minimum of inside boundaries and boundary incompatible for alternative prefix and inside 
     * boundaries for inferred subnet prefix.
     */
    
    inferredSubnetPrefix = 255;
    alternativeSubnetPrefix = 255;

    contraPivotNode = 0;

    int filterin = (SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES | 
                    SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY);
    
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            // Either nodes inside subnet boundaries or nodes marked for removal (boundary incompatibility) update alternativeSubnetPrefix
            if((*i)->prefix < alternativeSubnetPrefix)
            {
                alternativeSubnetPrefix = (*i)->prefix;
            }

            // Only nodes marked inside subnet boundaries update inferredSubnetPrefix
            if((*i)->nodeStatus == SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES)
            {
                if((*i)->prefix < inferredSubnetPrefix)
                {
                    inferredSubnetPrefix = (*i)->prefix;
                }
            }

            if((*i)->aliasStatus != SubnetSiteNode::UNKNOWN_ALIAS_SX)
            {
                contraPivotNode = (*i);
            }
        }
    }

    /**
     * If there is a contrapivot, extends the inferred prefix so that it covers the contrapivot
     * and makes the alternative just consisting of nodes marked inside subnet boundaries.
     */
    
    if(contraPivotNode != 0 && contraPivotNode->prefix < inferredSubnetPrefix)
    {
        alternativeSubnetPrefix = inferredSubnetPrefix;
        inferredSubnetPrefix = contraPivotNode->prefix;
    }

}

void SubnetSite::adjustLocalAreaSubnet(NetworkAddress &localAreaNetwork)
{
    this->inferredSubnetPrefix = localAreaNetwork.getPrefixLength();
}


void SubnetSite::adjustRemoteSubnet2(bool useLowerBorderAsWell)
{
    // See beginning of adjustRemoteSubnet() for comments

    bool exists[33];
    bool hasOverGrowth[33];
    bool hasLowerBorder[33];
    bool hasUpperBorder[33];
    for (int i = 0; i <= 32; ++i) 
    {
        exists[i] = false;
        hasOverGrowth[i] = false;
        hasLowerBorder[i] = false;
        hasUpperBorder[i] = false;
    }

    contraPivotNode = 0; // Contrapivot is meaningful only if it is covered by inferred prefix

    for(list<SubnetSiteNode *>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) == NULL)
            continue;
    
        exists[(int) ((*i)->prefix)] = true;
        if((*i)->nodeStatus == SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH)
        {
            hasOverGrowth[(int) ((*i)->prefix)] = true;
        }
        else
        {
            if((*i)->aliasStatus != SubnetSiteNode::UNKNOWN_ALIAS_SX)
            {
                contraPivotNode = (*i);
            }
        }
    }

    NetworkAddress tmpSubnet(InetAddress(0), 31); // Just to temporarily initialize
    bool foundLowerBorder;
    bool foundUpperBorder;
    unsigned char subnetPrefix = 30;
    
    //31 and /32 are set to hasLowerBorder false and hasUpperBorder false by default
    while(subnetPrefix > 0)
    {
        if(exists[(int) subnetPrefix] && !hasOverGrowth[(int) subnetPrefix])
        {
            tmpSubnet = NetworkAddress(this->pivotIPaddress, subnetPrefix);
            foundLowerBorder = this->contains(tmpSubnet.getLowerBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
            foundUpperBorder = this->contains(tmpSubnet.getUpperBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

            if(foundLowerBorder)
            {
                hasLowerBorder[(int) subnetPrefix] = true;
            }

            if(foundUpperBorder)
            {
                hasUpperBorder[(int) subnetPrefix] = true;
            }
        }
        subnetPrefix--;
    }

    bool hasBorder[33];

    if(useLowerBorderAsWell)
    {
        for (int i = 0; i <= 32; ++i)
        {
            hasBorder[i] = hasUpperBorder[i];
        }
    }
    else
    {
        for (int i = 0; i <= 32; ++i)
        {
            hasBorder[i] = (hasUpperBorder[i] || hasLowerBorder[i]);
        }
    }

    /**
     * inferredSubnetPrefix is the minimum prefix which does not cross the overgrowth nodes and 
     * does not have a border.
     */
    
    inferredSubnetPrefix = 255;
    for (int i = 0; i <= 32; ++i) 
    {
        if(exists[i] && !hasOverGrowth[i] && !hasBorder[i])
        {
            inferredSubnetPrefix = (unsigned char) i;
            break;
        }
    }

    /**
     * alternativeSubnetPrefix is the minimum prefix which does not cross the overgrowth nodes 
     * but covers all discovered IP addresses.
     */
    
    alternativeSubnetPrefix = 255;
    int minExistingNonOvergrowthPrefix = 0;
    for (int i = 0; i <= 32; ++i)
    {
        if(exists[i] && !hasOverGrowth[i])
        {
            minExistingNonOvergrowthPrefix = (unsigned char) i;
            break;
        }
    }

    /**
     * In case alternativeSubnetPrefix contains border IP address just expand it by one more 
     * level (i.e., decrease prefix length) regardless of checking if there is a discovered IP 
     * address at the new level.
     */
    
    if(!hasBorder[minExistingNonOvergrowthPrefix])
    {
        alternativeSubnetPrefix = minExistingNonOvergrowthPrefix;
    }
    else
    {
        if(!hasOverGrowth[minExistingNonOvergrowthPrefix - 1])
        {
            alternativeSubnetPrefix = minExistingNonOvergrowthPrefix - 1;
        }
    }

    /**
     * If there exists a contrapivot node but inferredSubnetPrefix does not cover it, expand 
     * inferredSubnetPrefix by one more level (i.e., decrease prefix length) so that it covers 
     * the contrapivot without any border addresses in case there are borders continue expanding 
     * it until getting rid of all borders.
     */
    
    if(inferredSubnetPrefix != 255)
    {
        unsigned char tmpPrefix;
        if(contraPivotNode != 0)
        {
            // Inferred prefix does not cover the contrapivot
            if(!hasOverGrowth[(int)contraPivotNode->prefix] && contraPivotNode->prefix < inferredSubnetPrefix)
            {
                tmpPrefix = inferredSubnetPrefix;
                for(int i = (int) contraPivotNode->prefix; i >= 0; i--)
                {
                    if(hasOverGrowth[i])
                    {
                        break;
                    }
                    else
                    {
                        if(!hasBorder[i])
                        {
                            tmpPrefix = (unsigned char) i;
                            i--;
                            
                            /**
                             * If there are larger existing subnets without having overgrowth and 
                             * upper borders, cover them.
                             */
                            
                            while(exists[i] && !hasOverGrowth[i] && !hasBorder[i])
                            {
                                tmpPrefix = (unsigned char) i;
                                i--;
                            }
                            break;
                        }
                    }
                }

                /**
                 * Switch alternative prefix with inferred prefix so that alternative is the most 
                 * compatible prefix length.
                 */
                
                if(tmpPrefix != inferredSubnetPrefix)
                {
                    alternativeSubnetPrefix = inferredSubnetPrefix;
                    inferredSubnetPrefix = tmpPrefix;
                }
            }
        }
        else
        {
            /**
             * If there is no contrapivot at all, it is better to assume that there must have 
             * been one but we did not discover it, hence use alternativeSubnetPrefix which 
             * encompasses all IP addresses as alternativeSubnetPrefix by switching them.
             */
            
            tmpPrefix = inferredSubnetPrefix;
            inferredSubnetPrefix = alternativeSubnetPrefix;
            alternativeSubnetPrefix = tmpPrefix;
        }
    }
    else
    {
        cout << "ERROR in adjust method of SubnetSite class inferredSubnetPrefix is 255" << endl;
    }
}


int SubnetSite::getInferredSubnetSize()
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->prefix >= inferredSubnetPrefix)
        {
            size++;
        }
    }

    return size;
}

int SubnetSite::getAlternativeSubnetSize()
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->prefix >= alternativeSubnetPrefix)
        {
            size++;
        }
    }

    return size;
}


unsigned char SubnetSite::getMinimumPrefixLength(int filterin)
{
    unsigned char minPrefix = 255; // Maximum possible value
    for(list<SubnetSiteNode *>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < minPrefix)
            {
                minPrefix = (*i)->prefix;
            }
        }
    }

    return minPrefix;
}

int SubnetSite::getSize(int filterin)
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            size++;
        }
    }

    return size;
}

bool SubnetSite::contains(InetAddress ip, int filterin)
{
    bool result = false;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->ip == ip)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

InetAddress SubnetSite::getInferredSubnetContraPivotAddress()
{
    InetAddress contraPivot(0);
    if(this->contraPivotNode != 0 && (this->contraPivotNode->prefix >= inferredSubnetPrefix))
    {
        contraPivot = this->contraPivotNode->ip;
    }
    return contraPivot;

}
InetAddress SubnetSite::getAlternativeSubnetContraPivotAddress()
{
    InetAddress contraPivot(0);
    if(this->contraPivotNode != 0 && (this->contraPivotNode->prefix >= alternativeSubnetPrefix))
    {
        contraPivot = this->contraPivotNode->ip;
    }
    return contraPivot;
}

string SubnetSite::getInferredNetworkAddressString()
{
    if(inferredSubnetPrefix > 32)
    {
        return string("");
    }
    else if(inferredSubnetPrefix == 32)
    {
        return (*(pivotIPaddress.getHumanReadableRepresentation())) + "/32";
    }
    else
    {
        NetworkAddress na(pivotIPaddress, inferredSubnetPrefix);
        return (*(na.getHumanReadableRepresentation()));
    }
}

string SubnetSite::getAlternativeNetworkAddressString()
{
    if(alternativeSubnetPrefix > 32)
    {
        return string("");
    }
    else if(alternativeSubnetPrefix == 32)
    {
        return (*(pivotIPaddress.getHumanReadableRepresentation())) + "/32";
    }
    else
    {
        NetworkAddress na(pivotIPaddress, alternativeSubnetPrefix);
        return (*(na.getHumanReadableRepresentation()));
    }
}

bool SubnetSite::hasAlternativeSubnet()
{
    return ((alternativeSubnetPrefix != 255) && (alternativeSubnetPrefix != inferredSubnetPrefix));
}

bool SubnetSite::compare(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned char prefixLength1 = ss1->getInferredSubnetPrefixLength();
    unsigned long int prefix1 = (ss1->getPivotAddress()).getULongAddress();
    prefix1 = prefix1 >> (32 - prefixLength1);
    prefix1 = prefix1 << (32 - prefixLength1);
    
    unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
    unsigned long int prefix2 = (ss2->getPivotAddress()).getULongAddress();
    prefix2 = prefix2 >> (32 - prefixLength2);
    prefix2 = prefix2 << (32 - prefixLength2);
    
    bool result = false;
    if (prefix1 < prefix2)
        result = true;
    return result;
}

bool SubnetSite::compareRoutes(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned short size1 = ss1->getRefinementRouteSize();
    unsigned short size2 = ss2->getRefinementRouteSize();

    if(size1 == 0 && size2 == 0)
        return compare(ss1, ss2);
    
    if (size1 < size2)
        return true;
    return false;
}

void SubnetSite::prepareForRefinementTTLs()
{
    unsigned char shortestTTL = 0, greatestTTL = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        SubnetSiteNode *cur = (*i);
        
        /*
         * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
         * This condition is present in the original ExploreNET.
         */
        
        if(cur != NULL && cur->prefix >= this->inferredSubnetPrefix)
        {
            if(greatestTTL == 0 || cur->TTL > greatestTTL)
                greatestTTL = cur->TTL;
            
            if(shortestTTL == 0 || cur->TTL < shortestTTL)
                shortestTTL = cur->TTL;
        }
    }
    
    this->refinementTTL1 = shortestTTL;
    this->refinementTTL2 = greatestTTL;
}

void SubnetSite::prepareForRefinement()
{
    // If there is only one IP, cannot tell if it is a (contra-)pivot
    if(IPlist.size() == 1)
    {
        this->refinementStatus = SubnetSite::INCOMPLETE_SUBNET;
        this->refinementTTL1 = IPlist.front()->TTL;
        this->refinementTTL2 = IPlist.front()->TTL;
        return;
    }
    
    this->prepareForRefinementTTLs();
    unsigned char shortestTTL = this->refinementTTL1;
    unsigned char greatestTTL = this->refinementTTL2;
    
    // Same TTL everywhere: subnet is incomplete for sure
    if(shortestTTL == greatestTTL)
    {
        this->refinementStatus = SubnetSite::INCOMPLETE_SUBNET;
    }
    // 2 TTLs with a difference of 1: subnet might be accurate
    else if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        // Checks a list a second time to confirm there is only one smallestTTL (= contra-pivot)
        bool foundContrapivot = false;
        InetAddress tempContrapivot;
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            
            if(cur != NULL && cur->TTL == shortestTTL)
            {
                if(!foundContrapivot)
                {
                    tempContrapivot = cur->ip;
                    foundContrapivot = true;
                }
                else
                {
                    this->refinementStatus = SubnetSite::ODD_SUBNET;
                    return;
                }
            }
        }
        this->refinementStatus = SubnetSite::ACCURATE_SUBNET;
        this->refinementContrapivot = tempContrapivot;
    }
    // Any other case: subnet is odd
    else
    {
        this->refinementStatus = SubnetSite::ODD_SUBNET;
    }
}

void SubnetSite::recomputeRefinementStatus()
{
    // Not ACCURATE: aborts (we only double check status for ACCURATE subnets)
    if(this->refinementStatus != SubnetSite::ACCURATE_SUBNET)
    {
        return;
    }
    
    // Double-checks shortest/greatest TTLs
    this->prepareForRefinementTTLs();
    unsigned short shortestTTL = this->refinementTTL1;
    unsigned short greatestTTL = this->refinementTTL2;

    // 2 TTLs with a difference of 1: subnet might be accurate
    if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        unsigned short nbContrapivot = 0;
        InetAddress smallestContrapivot = this->refinementContrapivot;
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
            this->refinementStatus = SubnetSite::ACCURATE_SUBNET;
        else
        {
            this->refinementStatus = SubnetSite::ODD_SUBNET;
            this->refinementContrapivot = smallestContrapivot;
        }
    }
    // Any other case: subnet is odd
    else
    {
        this->refinementStatus = SubnetSite::ODD_SUBNET;
    }
}

bool SubnetSite::containsAddress(InetAddress i)
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
        if((*i) != NULL && (*i)->ip == li)
            return true;
    }
    return false;
}

InetAddress SubnetSite::getRefinementPivot()
{
    // Not refined yet or undefined: aborts
    if(this->refinementStatus == SubnetSite::NOT_PREPARED_YET ||
       this->refinementStatus == SubnetSite::UNDEFINED_SUBNET ||
       this->refinementStatus == SubnetSite::INCOMPLETE_SUBNET)
    {
        return InetAddress(0);
    }
    
    // Shadow subnet: gets the IP of the first "visible" address
    if(this->refinementStatus == SubnetSite::SHADOW_SUBNET)
    {
        return IPlist.front()->ip;
    }
    
    unsigned short shortestTTL = this->refinementTTL1;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->TTL == shortestTTL + 1)
        {
            return (*i)->ip;
        }
    }
    
    // Null if no result (very unlikely)
    return InetAddress(0);
}

bool SubnetSite::hasCompleteRoute()
{
    for(unsigned short i = 0; i < this->refinementRouteSize; ++i)
    {
        if(this->refinementRoute[i] == InetAddress("0.0.0.0"))
        {
            return false;
        }
    }
    return true;
}

bool SubnetSite::isAnArtifact()
{
    if(this->inferredSubnetPrefix >= (unsigned char) 32)
        return true;
    return false;
}

string SubnetSite::refinedToString()
{
    stringstream ss;
    
    if((this->refinementStatus == SubnetSite::ACCURATE_SUBNET || 
        this->refinementStatus == SubnetSite::SHADOW_SUBNET || 
        this->refinementStatus == SubnetSite::ODD_SUBNET) && 
        this->refinementRoute != 0)
    {
        ss << this->getInferredNetworkAddressString() << "\n";
        if(this->refinementStatus == SubnetSite::ACCURATE_SUBNET)
            ss << "ACCURATE\n";
        else if(this->refinementStatus == SubnetSite::SHADOW_SUBNET)
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
        
            /*
             * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
             * This condition is present in the original ExploreNET.
             */
            
            if((*i)->prefix >= this->inferredSubnetPrefix)
            {
                if(guardian)
                    ss << ", ";
                else
                    guardian = true;
            
                ss << (*i)->ip << " - " << (unsigned short) (*i)->TTL;
            }
            
            previous = (*i)->ip;
        }
        ss << "\n";
        
        // Writes route
        guardian = false;
        for(unsigned int i = 0; i < this->refinementRouteSize; i++)
        {
            if(guardian)
                ss << ", ";
            else
                guardian = true;
            
            ss << this->refinementRoute[i];
            
            if(this->hasRepairedRoute() && this->routeRepairMask[i])
                ss << " [Repaired]";
        }
        ss << "\n";
    }
    
    return ss.str();
}

bool SubnetSite::isCredible()
{
    bool credibility = false;
    
    unsigned short baseTTL = (unsigned short) this->refinementTTL1;
    unsigned short diffTTL = (unsigned short) this->refinementTTL2 - baseTTL;
    
    if(diffTTL == 0)
    {
        return false;
    }
    
    unsigned int *interfacesByTTL = new unsigned int[diffTTL + 1];
    for (unsigned short i = 0; i < (diffTTL + 1); i++)
        interfacesByTTL[i] = 0;
    
    unsigned int totalInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        /*
         * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
         * This condition is present in the original ExploreNET.
         *
         * (*i) != NULL is there just in case, to avoid potential seg fault (rare, but seems to
         * occur on some PlanetLab nodes for some reason).
         */
        
        if((*i) != NULL && (*i)->prefix >= this->inferredSubnetPrefix)
        {
            unsigned short offset = (unsigned short) (*i)->TTL - baseTTL;
            if(offset <= diffTTL)
            {
                interfacesByTTL[offset]++;
                totalInterfaces++;
            }
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
    else if(diffTTL > 1)
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
