/*
 * SubnetSiteSet.h
 *
 *  Created on: Oct 9, 2014
 *      Author: grailet
 *
 * A simple class to gather several subnet sites and organize them. This class, in ExploreNET++, 
 * is a simplified version of what can be found in TreeNET (Note: ExploreNET++ was developped in 
 * November 2015).
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

    SubnetSiteSet();
    ~SubnetSiteSet();
    
    // Accessor to the list
    inline list<SubnetSite*> *getSubnetList() { return &siteList; }
    
    // Method to add a new subnet to the set
    void addSite(SubnetSite *ss);
    
    // Method to write the complete set in an output file of a given name.
    void outputAsFile(string filename);
    
private:

    // Sites are stored with a list
    list<SubnetSite*> siteList;
};

#endif /* SUBNETSITESET_H_ */
