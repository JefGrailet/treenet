/*
 * SubnetSiteSet.h
 *
 *  Created on: Oct 9, 2014
 *      Author: grailet
 *
 * A simple class to gather several subnet sites and organize them before further discovery
 * steps. Sites registered in the set are sorted according to their CIDR notation (small IPs 
 * first) and when newly sites happen to include a previously registered site, the "old" site is 
 * removed. Reciprocally, when a site to add is included in an already registered site, the new 
 * site is not inserted but its IPs missing from the already registered site are stored within 
 * the object of the latter.
 */

#ifndef SUBNETSITESET_H_
#define SUBNETSITESET_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "../../common/inet/InetAddress.h"
#include "SubnetSite.h"

class SubnetSiteSet
{
public:

    // Possible results when adding a site
    enum UpdateResult
    {
        KNOWN_SUBNET, // Site already in the set (in practice, for /32 subnets)
        SMALLER_SUBNET, // Site already in the set, but with bigger/equivalent prefix
        BIGGER_SUBNET, // Site already in the set, but with smaller prefix
        NEW_SUBNET // Site not in the set (therefore inserted)
    };

    SubnetSiteSet();
    ~SubnetSiteSet();
    
    // Accessor to the list
    inline list<SubnetSite*> *getSubnetSiteList() { return &siteList; }
    
    // Method to get a subnet which contains a given IP (also, to check if an IP is covered)
    SubnetSite *getSubnetContaining(InetAddress ip);
    
    /*
     * Method to test if a an hypothetical subnet (represented with its borders and TTL to reach 
     * it) is compatible with this set. A subnet being compatible with the set means that:
     * -either it does not overlap any subnet within the set,
     * -either it only overlaps subnets which the TTL used to reach them is similar.
     * True is returned if the given subnet is compatible. Otherwise, false is returned.
     *
     * For accurracy, a boolean telling if we should check both TTL-1 and TTL+1 to tell if the
     * TTL is similar is required. It should be true when there is only one live interface in the 
     * hypothetical subnet. A last boolean (shadowExpansion) prevents from encompassing 
     * ACCURATE/ODD subnets while expanding a SHADOW one.
     */
     
    bool isCompatible(InetAddress lowerBorder, 
                      InetAddress upperBorder, 
                      unsigned char TTL, 
                      bool beforeAndAfter, 
                      bool shadowExpansion);
    
    // Method to add a new subnet to the set
    unsigned short addSite(SubnetSite *ss);
    
    // Method to get the longest route within the set.
    unsigned short getLongestRoute();
    
    // Method to sort the subnets by increasing route size (when known).
    void sortByRoute();
    
    /**
     * Method to obtain a incomplete subnet for refinement purpose. If there is no incomplete
     * subnet, it will return NULL.
     *
     * N.B.: if a subnet is returned, it is also removed from the set (it will be added after
     * refining to benefit from the merging mechanics of addSite()).
     */
    
    SubnetSite *getIncompleteSubnet();
    
    // Similar method but returning exclusively SHADOW subnets.
    SubnetSite *getShadowSubnet();
    
    /*
     * getValidSubnet() is the dual of getIncompleteSubnet(). It returns a ACCURATE/ODD/SHADOW 
     * subnet and removes it from the set in the process. The single boolean parameter is used 
     * to discriminate subnets which traceroute is complete (i.e. no "0.0.0.0" in it) from others. 
     * It is set to true by default.
     */
    
    SubnetSite *getValidSubnet(bool completeRoute = true);
    
    // Method to write the complete set in an output file of a given name.
    void outputAsFile(string filename);
    
private:

    // Sites are stored with a list
    list<SubnetSite*> siteList;
};

#endif /* SUBNETSITESET_H_ */
