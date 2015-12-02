/*
 * SubnetRefiner.h
 *
 *  Created on: Oct 23, 2014
 *      Author: grailet
 *
 * This class gathers refinement methods to call on subnets after the scanning (in other words,
 * it is a form of postprocessing). The refinement methods are described just below.
 *
 * 1) Subnet filling
 *
 * During subnet inference, it is not rare to see that the IP list of some subnet misses some 
 * IPs which were previously considered as responsive. The goal of filling is to double check the 
 * list such that responsive interfaces encompassed by inferred subnets are listed in them as 
 * well at the end of the subnet refinement.
 *
 * 2) Subnet expansion
 *
 * This method consists in overgrowing a subnet which does not list a contra-pivot (i.e. all 
 * listed IPs are at the same hop count, and there must be exactly one IP with hop count - 1,
 * called contra-pivot) and probing all non-listed IPs with a TTL smaller than one hop than 
 * the smallest TTL to get an echo reply from a pivot. Once the contra-pivot is found, the 
 * check stops overgrowth and edits the subnet so that its prefix is the prefix for which the
 * contra-pivot was found (contra-pivot is added in the listed IPs as well). When the method 
 * overgrowths the subnet too much, it stops as well.
 */

#ifndef SUBNETREFINER_H_
#define SUBNETREFINER_H_

#include "../TreeNETEnvironment.h"
#include "../structure/SubnetSite.h"
#include "../structure/SubnetSiteSet.h"
#include "ProbesDispatcher.h"

class SubnetRefiner
{
public:

    static const unsigned short LOWEST_PREFIX_ALLOWED = 20;
    static const unsigned short MAX_CONTRAPIVOT_CANDIDATES = 5;

    // Constructor, destructor
    SubnetRefiner(TreeNETEnvironment *env);
    ~SubnetRefiner();
    
    // Refinement methods
    void expand(SubnetSite *ss);
    void fill(SubnetSite *ss);
    
    /*
     * Expansion method to use for shadow subnets at the end of the scanning ONLY; give them 
     * the greatest possible size without colliding with other (accurate/odd) subnets.
     */
    
    void shadowExpand(SubnetSite *ss);
    
private:

    // Pointer to the environment (provides access to the subnet set and prober parameters)
    TreeNETEnvironment *env;
}; 

#endif /* SUBNETREFINER_H_ */
