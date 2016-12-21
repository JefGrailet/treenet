/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * From ExploreNET v2.1, edited by J.-F. Grailet (starting from October 2014) for the needs of 
 * TreeNET and adapted for TreeNET Reader, and later, TreeNET "Forester" (first released with 
 * v3.0).
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
// #include "../bipartite/BipartiteSubnet.h" TODO put this back

class SubnetSite
{
public:
    
    // Possible status for a subnet before/after refinement
    enum RefinementStatus
    {
        UNDEFINED_SUBNET, // Failed to parse the subnet status
        ACCURATE_SUBNET, // Subnet contains its contra-pivot
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
        SHADOW_SUBNET // Subnet most probably exists, but we cannot find (contra)pivot(s) for sure**
    };
    
    /*
     * **expanding this subnet collides it with accurate subnets for which the TTLs do not match.
     * For example: we expand a /32 subnet for which the only IP is at 9, but expanding it to /29
     * makes it cover another accurate subnet for which the contrapivot is at 7 and pivot(s) at 
     * 8. Therefore making this subnet expand further will eventually lead to merging it with 
     * accurate subnets, resulting in one big wrong subnet. However, we can still infer that the
     * subnet we are expanding exists to some extent. It is then labelled as a "shadow" subnet.
     */

    // Constructor/destructor
    SubnetSite();
    ~SubnetSite();

    list<SubnetSiteNode*> *getSubnetIPList() { return &IPlist; }
    unsigned int getNbResponsiveIPs() { return IPlist.size(); }
    void clearIPlist();

    // Basic setters and accessers
    inline void setInferredSubnetBaseIP(InetAddress sbi) { this->inferredSubnetBaseIP = sbi; }
    inline void setInferredSubnetPrefixLength(unsigned char spl) { this->inferredSubnetPrefix = spl; }
    
    inline InetAddress getInferredSubnetBaseIP() { return this->inferredSubnetBaseIP; }
    inline unsigned char getInferredSubnetPrefixLength() { return this->inferredSubnetPrefix; }
    
    inline int getTotalSize() { return IPlist.size(); }
    inline NetworkAddress getInferredNetworkAddress() { return NetworkAddress(this->inferredSubnetBaseIP, this->inferredSubnetPrefix); }
    
    // Accessers implemented in SubnetSite.cpp (longer implementation)
    int getInferredSubnetSize();
    string getInferredNetworkAddressString();

    // Comparison methods for sorting purposes (one compares lower bounds, the other the routes)
    static bool compare(SubnetSite *ss1, SubnetSite *ss2);
    static bool compareRoutes(SubnetSite *ss1, SubnetSite *ss2);
    
    // Method to insert a new interface in the IP list
    inline void insert(SubnetSiteNode *ssn) { this->IPlist.push_back(ssn); }
    
    // Method to recompute the shortest/greatest TTL after parsing
    void completeRefinedData();
    
    // Methods to adapt the TTLs of the nodes after recomputing the routes
    void adaptTTLs(unsigned short pivotTTL);
    
    // Setter and accessers to the refined data
    inline void setStatus(unsigned short s) { this->status = s; }
    inline unsigned short getStatus() { return this->status; }
    inline InetAddress &getContrapivot() { return this->contrapivot; }
    inline unsigned char getShortestTTL() { return this->TTL1; }
    inline unsigned char getGreatestTTL() { return this->TTL2; }
    
    // Method to know if a given InetAddress is within this subnet boundaries
    bool contains(InetAddress i);
    
    // Method to know if a given InetAddress appears in the live interfaces of this subnet
    bool hasLiveInterface(InetAddress li);
    
    // Method to know if a given InetAddress appears in the route to this subnet at a given TTL
    bool hasRouteLabel(InetAddress rl, unsigned short TTL);
    
    // Method to obtain one or several (Contra-)Pivot addresses of this subnet
    InetAddress getPivot();
    list<InetAddress> getPivotAddresses(unsigned short max);
    unsigned short countContrapivotAddresses();
    list<InetAddress> getContrapivotAddresses();
    
    // Methods for route manipulation (merged with v3.0)
    inline void setRouteSize(unsigned short rs) { this->routeSize = rs; }
    inline void setRoute(RouteInterface *route) { this->route = route; }
    inline unsigned short getRouteSize() { return this->routeSize; }
    inline RouteInterface *getRoute() { return this->route; }
    bool hasCompleteRoute(); // Returns true if there is a route AND without "holes" (i.e. 0.0.0.0)
    
    // toString() method, only available for refined (odd/accurate) subnets, null otherwise 
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
    
    // Method to obtain the capacity (i.e. max number of interfaces in this subnet)
    unsigned int getCapacity();
    
    // Accessor/setter to the bipartite element (+ test method) TODO: put this back
    // inline bool hasBipEquivalent() { return this->bipSubnet != NULL; }
    // inline void setBipEquivalent(BipartiteSubnet *bipSubnet) { this->bipSubnet = bipSubnet; }
    // inline BipartiteSubnet *getBipEquivalent() { return this->bipSubnet; }
    
    // Method to adapt the route for grafting (exclusive to Forester)
    bool matchRoutePrefix(unsigned short sPrefix, InetAddress *prefix);
    void adaptRoute(unsigned short offset, unsigned short sNew, InetAddress *newPrefix);
    
private:
    int getSize(int filterin);
    
    // Private fields
    list<SubnetSiteNode*> IPlist;
    InetAddress inferredSubnetBaseIP;
    unsigned char inferredSubnetPrefix;
    
    // Fields dedicated to refined data
    unsigned short status;
    InetAddress contrapivot;
    unsigned short TTL1, TTL2; // Shortest and greatest TTL for this subnet
    unsigned short routeSize;
    RouteInterface *route;
    
    // Corresponding bipartite element TODO: put this back
    // BipartiteSubnet *bipSubnet;
    
};

#endif /* SUBNETSITE_H_ */
