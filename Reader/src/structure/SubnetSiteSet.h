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
 *
 * N.B.: for TreeNET Reader, it is not always relevant to apply the merging policy (e.g. if the 
 * input is already sorted, with no overlapping). Therefore, additionnal methods are defined to 
 * insert new subnets without applying that policy, speeding up the insertion.
 */

#ifndef SUBNETSITESET_H_
#define SUBNETSITESET_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "../common/inet/InetAddress.h"
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

    /*
     * Method to test if a an hypothetical subnet (represented with its borders) is compatible 
     * with this set. A subnet being compatible with the set means that it does not overlap any 
     * subnet within the set (N.B.: in the full version of TreeNET, this method has more 
     * parameters and is more complex because a subnet that can be merged with another is also
     * considered as compatible; here, there is no merging policy).
     */
     
    bool isCompatible(InetAddress lowerBorder, InetAddress upperBorder);
    
    // Method to add a new subnet to the set (with merging policy and sorting).
    unsigned short addSite(SubnetSite *ss);
    
    // Method to add a new subnet with neither merging, nor sorting.
    void addSiteNoRefinement(SubnetSite *ss);
    
    // Method to sort the set (to use in complement with addSiteNoRefinement, at the end).
    void sortSet();
    
    // Method to get the longest route within the set.
    unsigned short getLongestRoute();
    
    // Method to sort the subnets by increasing route size (when known).
    void sortByRoute();
    
    // getValidSubnet() returns an ACCURATE/ODD/SHADOW subnet while removing it from the set.
    SubnetSite *getValidSubnet();
    
    // Method to write the complete set in an output file of a given name.
    void outputAsFile(string filename);
    
private:

    // Sites are stored with a list
    list<SubnetSite*> siteList;
};

#endif /* SUBNETSITESET_H_ */
