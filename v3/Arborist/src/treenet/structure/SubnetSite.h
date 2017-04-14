/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Modifications brought by J.-F. Grailet:
 * -From October 2014 to roughly end of 2015: improved coding style, new fields and methods 
 *  suited for TreeNET.
 * -Late Augustus 2016: remove the override of << operator, because it is no longer useful even 
 *  in the new debug/verbose utilities.
 * -Sept 8, 2016: slight refactoring of the class to benefit from the addition of the class 
 *  RouteInterface. There is now a single route array, and no longer two (one for the IPs along 
 *  the route, and a second for the repairment mask).
 * -Mar 27, 2017: addition of a second route for post-processing. It replaces the observed route 
 *  when available.
 */

#ifndef SUBNETSITE_H_
#define SUBNETSITE_H_

#include <list>
using std::list;
#include <iostream>
using std::ostream;
#include <string>
using std::string;

#include "SubnetSiteNode.h"
#include "RouteInterface.h"
#include "../../common/inet/NetworkAddress.h"

class SubnetInferrer;

class SubnetSite
{
public:

    friend class SubnetInferrer;
    
    // Possible status for a subnet before/after refinement
    enum RefinementStatus
    {
        NOT_PREPARED_YET, // Subnet exists but is not ready for refinement
        ACCURATE_SUBNET, // Subnet contains its contra-pivot
        INCOMPLETE_SUBNET, // Subnet contains only one IP or all IPs are at the same hop distance
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
        SHADOW_SUBNET, // Subnet most probably exists, but we cannot find (contra)pivot(s) for sure**
        UNDEFINED_SUBNET // Refinement failed to improve this subnet
    };
    
    /*
     * **expanding this subnet collides it with accurate subnets for which the TTLs do not match.
     * For example: we expand a /32 subnet for which the only IP is at 9, but expanding it to /29
     * makes it cover another accurate subnet for which the contrapivot is at 7 and pivot(s) at 
     * 8. Therefore making this subnet expand further will eventually lead to merging it with 
     * accurate subnets, resulting in one big wrong subnet. However, we can still infer that the
     * subnet we are expanding exists. It is then labelled as a "shadow" subnet.
     */

    // Constructor/destructor
    SubnetSite();
    ~SubnetSite();

    list<SubnetSiteNode*> *getSubnetIPList() { return &IPlist; }
    bool hasAlternativeSubnet();
    void clearIPlist();

    // Accessers (straightforward implementation)
    inline unsigned int getSubnetPositioningCost() { return this->spCost; }
    inline unsigned int getSubnetInferenceCost() { return this->siCost; }
    inline int getMergeAmount() { return this->mergeAmount; }
    inline InetAddress &getTargetAddress() { return this->targetIPaddress; }
    inline InetAddress &getPivotAddress() { return this->pivotIPaddress; } // ! Pivot during ExploreNET inference
    inline InetAddress &getIngressInterfaceAddress() { return this->prevSiteIPaddress; }
    inline int getSubnetHopDistance() { return this->prevSiteIPaddressDistance; }
    inline unsigned char getInferredSubnetPrefixLength() { return this->inferredSubnetPrefix; }
    inline unsigned char getAlternativeSubnetPrefixLength() { return this->alternativeSubnetPrefix; }
    inline int getTotalSize() { return IPlist.size(); }
    
    inline NetworkAddress getInferredNetworkAddress() { return NetworkAddress(this->pivotIPaddress, this->inferredSubnetPrefix); }
    inline NetworkAddress getAlternativeNetworkAddress() { return NetworkAddress(this->pivotIPaddress, this->alternativeSubnetPrefix); }
    
    // Accessers implemented in SubnetSite.cpp (longer implementation)
    InetAddress getInferredSubnetContraPivotAddress(); // Returns alias if subnet inference found one
    InetAddress getAlternativeSubnetContraPivotAddress();
    int getInferredSubnetSize();
    int getAlternativeSubnetSize();
    string getInferredNetworkAddressString();
    string getAlternativeNetworkAddressString();
    
    // Handler of the merge amount value
    inline void incMergeAmount() { this->mergeAmount++; }
    inline void setMergeAmount(unsigned int ma) { this->mergeAmount = ma; }

    // Comparison methods for sorting purposes (one compares lower bounds, the other the routes)
    static bool compare(SubnetSite *ss1, SubnetSite *ss2);
    static bool compareRoutes(SubnetSite *ss1, SubnetSite *ss2);
    
    // Method to insert a new interface in the IP list (made public for SubnetRefiner class)
    inline void insert(SubnetSiteNode *ssn) { this->IPlist.push_front(ssn); }
    
    // Method to call after inference is completed to prepare refinement steps
    void prepareForRefinement();
    
    // Accessers to refinement data
    inline unsigned short getStatus() { return this->refinementStatus; }
    inline InetAddress &getContrapivot() { return this->refinementContrapivot; }
    inline unsigned char getShortestTTL() { return this->refinementTTL1; }
    inline unsigned char getGreatestTTL() { return this->refinementTTL2; }
    
    // Setters for refinement data and pivot/prefix edition
    inline void setStatus(unsigned short rs) { this->refinementStatus = rs; }
    inline void setContrapivot(InetAddress &rc) { this->refinementContrapivot = rc; }
    inline void setShortestTTL(unsigned char rst) { this->refinementTTL1 = rst; }
    inline void setGreatestTTL(unsigned char rgt) { this->refinementTTL2 = rgt; }
    inline void setPivotAddress(InetAddress &pivot) { this->pivotIPaddress = pivot; }
    inline void setInferredSubnetPrefixLength(unsigned char ispl) { this->inferredSubnetPrefix = ispl; }

    // Method to recompute the refinement status at certain points
    void recomputeRefinementStatus();
        
    // Method to know if a given InetAddress is within this subnet boundaries
    bool contains(InetAddress i);
    
    // Method to know if a given InetAddress appears in the live interfaces of this subnet
    bool hasLiveInterface(InetAddress li);
    
    // Method to obtain a pivot address of this subnet after refinement
    InetAddress getPivot();
    
    // Basic methods for route manipulation.
    inline void setRouteTarget(InetAddress rt) { this->routeTarget = rt; }
    inline void setRouteSize(unsigned short rs) { this->routeSize = rs; }
    inline void setRoute(RouteInterface *route) { this->route = route; }
    inline InetAddress getRouteTarget() { return this->routeTarget; }
    inline unsigned short getRouteSize() { return this->routeSize; }
    inline RouteInterface *getRoute() { return this->route; }
    inline bool hasValidRoute() { return (this->routeSize > 0 && this->route != NULL); }
    
    // Next methods assume the user previously checked there is a valid route.
    bool hasCompleteRoute(); // Returns true if the route has no "holes" (i.e. 0.0.0.0)
    bool hasIncompleteRoute(); // Dual operation (true if the route has 0.0.0.0's)
    unsigned short countMissingHops(); // Returns amount of 0.0.0.0's
    
    // Additionnal and optional post-processed route than can be set in TreeNET v3.2
    inline void setProcessedRouteSize(unsigned short prs) { this->processedRouteSize = prs; }
    inline void setProcessedRoute(RouteInterface *pRoute) { this->processedRoute = pRoute; }
    inline unsigned short getProcessedRouteSize() { return this->processedRouteSize; }
    inline RouteInterface *getProcessedRoute() { return this->processedRoute; }

    // Method to get the final route (priority: processed then observed, NULL if nothing)
    RouteInterface *getFinalRoute(unsigned short *finalRouteSize);
    
    /*
     * Method to test if the subnet is an "artifact", i.e., a subnet which was inferred as a /32 
     * and still is after refining. While there are rare occurrences of actual /32 "in the wild", 
     * their presence in the subsequent steps of TreeNET (tree construction, alias resolution) can 
     * be a problem for both interpretation of the data and behavior of the program (it can crash 
     * when one or several /32 subnet(s) remain(s)).
     */
    
    bool isAnArtifact();
    
    // toString() method, only available for refined subnets, otherwise it returns an empty string 
    string toString();
    
    /*
     * Special method to evaluate the credibility of the subnet. Indeed, for several reasons,
     * like redirections or asymetric paths in load balancers, an ACCURATE or ODD subnet might 
     * not be a good measure and can have outliers. To tell which subnet is a good measure, 
     * this method computes the amount of live interfaces in the subnet and computes their 
     * proportion. Ideally, there should be one IP at the shortest TTL and all the others at
     * this TTL+1, but depending on the proportions, we can consider a subnet will several
     * candidates for contra-pivots is credible.
     */
    
    bool isCredible();

private:
    int getSize(int filterin);
    
    // Filter determines what type of site nodes (i.e., node status) should be included in minimum search
    unsigned char getMinimumPrefixLength(int filterin);

    void markSubnetOvergrowthElements(unsigned char barrierPrefix, int filterin);
    void adjustLocalAreaSubnet(NetworkAddress &localAreaNetwork); // Added later to make sure LAN prefix is set forcefully
    void adjustRemoteSubnet2(bool useLowerBorder);
    void adjustRemoteSubnet();
    void markSubnetBoundaryIncompatibileElements(unsigned char basePrefix, int filterin);
    bool contains(InetAddress ip, int filterin);
    void prepareForRefinementTTLs();
    
    // Private fields
    list<SubnetSiteNode*> IPlist;
    unsigned int spCost; // Subnet positioning cost (SPC)
    unsigned int siCost; // Subnet inference cost (SIC)
    unsigned int mergeAmount; // Amount of merge operations to improve this subnet (MA)

    InetAddress targetIPaddress;
    InetAddress pivotIPaddress; // SiteRecord.destination
    InetAddress prevSiteIPaddress; // Ingress interface
    unsigned char prevSiteIPaddressDistance;

    unsigned char inferredSubnetPrefix;
    unsigned char alternativeSubnetPrefix;
    SubnetSiteNode *contraPivotNode;
    
    // Fields dedicated to refinement data
    unsigned short refinementStatus;
    InetAddress refinementContrapivot;
    unsigned short refinementTTL1, refinementTTL2; // Shortest and greatest TTL for this subnet
    
    // Route fields (observed + post-processed)
    InetAddress routeTarget; // To keep track of the target IP used during traceroute
    unsigned short routeSize, processedRouteSize;
    RouteInterface *route, *processedRoute;
};

#endif /* SUBNETSITE_H_ */
