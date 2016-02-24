/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * From ExploreNET v2.1, edited by J.-F. Grailet (starting from October 2014) for the needs of 
 * TreeNET and adapted for TreeNET Reader.
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
#include "../common/inet/NetworkAddress.h"
#include "../bipartite/BipartiteSubnet.h"

class SubnetSite
{
public:

    // Overriden << operator, only used by debug mode (see also SubnetSite.h)
    friend ostream &operator<<(ostream &out, SubnetSite &ss)
    {
        out << "Subnet: " << ss.getInferredNetworkAddressString() << endl;

        for(list<SubnetSiteNode *>::const_iterator i = ss.IPlist.begin(); i != ss.IPlist.end(); ++i)
        {
            out << "\t" << *(*i) << endl;
        }

        return out;
    }
    
    // Possible status for a subnet before/after refinement
    enum RefinementStatus
    {
        UNDEFINED_SUBNET, // Failed to parse the subnet status
        ACCURATE_SUBNET, // Subnet contains its contra-pivot
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
        SHADOW_SUBNET // Subnet most probably exists, but we cannot find (contra)pivot(s) for sure**
    };
    
    // Possible status for interfaces found in the route to the subnet
    enum RouteInterfaceStatus
    {
        OBSERVED_INTERFACE, // Directly observed by TreeNET
        REPAIRED_INTERFACE, // Appeared after repairment by TreeNET
        PREDICTED_INTERFACE // Predicted while transplanting the subnet (merging in TreeNET Reader)
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
    
    // Setters for (some) refined data fields
    inline void setStatus(unsigned short s) { this->status = s; }
    inline void setRouteSize(unsigned short rs) { this->routeSize = rs; }
    inline void setRoute(InetAddress *route) { this->route = route; }
    inline void setRouteEditMask(unsigned short *mask) { this->routeEditMask = mask; }
    
    // Method to recompute the shortest/greatest TTL after parsing
    void completeRefinedData();
    
    // Methods to adapt the TTLs of the nodes after recomputing the routes
    void adaptTTLs(unsigned short pivotTTL);
    
    // Accessers to the refined data
    inline unsigned short getStatus() { return this->status; }
    inline InetAddress &getContrapivot() { return this->contrapivot; }
    inline unsigned char getShortestTTL() { return this->TTL1; }
    inline unsigned char getGreatestTTL() { return this->TTL2; }
    inline unsigned short getRouteSize() { return this->routeSize; }
    inline InetAddress *getRoute() { return this->route; }
    inline bool hasEditedRoute() { return this->routeEditMask != NULL; }
    inline unsigned short *getRouteEditMask() { return this->routeEditMask; }
    
    // Method to know if a given InetAddress is within this subnet boundaries
    bool contains(InetAddress i);
    
    // Method to know if a given InetAddress appears in the live interfaces of this subnet
    bool hasLiveInterface(InetAddress li);
    
    // Method to know if a given InetAddress appears in the route to this subnet at a given TTL
    bool hasRouteLabel(InetAddress rl, unsigned short TTL);
    
    // Method to obtain one or several (Contra-)Pivot addresses of this subnet
    InetAddress getPivotAddress();
    list<InetAddress> getPivotAddresses(unsigned short max);
    unsigned short countContrapivotAddresses();
    list<InetAddress> getContrapivotAddresses();
    
    // Method to know if the subnet has a route, and if yes, if it is complete (i.e. no 0.0.0.0)
    bool hasCompleteRoute();
    
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
    
    // Accessor/setter to the bipartite element (+ test method)
    inline bool hasBipEquivalent() { return this->bipSubnet != NULL; }
    inline void setBipEquivalent(BipartiteSubnet *bipSubnet) { this->bipSubnet = bipSubnet; }
    inline BipartiteSubnet *getBipEquivalent() { return this->bipSubnet; }
    
    // Methods related to transplantation (exclusive to TreeNET Reader)
    bool matchRoutePrefix(unsigned short sPrefix, InetAddress *prefix);
    void transplantRoute(unsigned short offset, unsigned short sNew, InetAddress *newPrefix);
    
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
    InetAddress *route;
    unsigned short *routeEditMask;
    
    // Corresponding bipartite element
    BipartiteSubnet *bipSubnet;
    
};

#endif /* SUBNETSITE_H_ */
