/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (starting from October 2014) to improve coding style, study the code 
 * and implement new fields and methods for the needs of TreeNET.
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
#include "../../common/inet/NetworkAddress.h"

class SubnetInferrer;

class SubnetSite
{
public:

	friend class SubnetInferrer;
	
	// Overriden << operator, only used by debug mode (see also SubnetSite.h)
	friend ostream &operator<<(ostream &out, SubnetSite &ss)
	{
		out << "Subnet: " << ss.getInferredNetworkAddressString()
	    << "  Alternative Subnet: " << ss.getAlternativeNetworkAddressString()
		<< endl << "\t"
		<< "targetIP: " << *(ss.targetIPaddress.getHumanReadableRepresentation())
		<< " pivotIP: " << *(ss.pivotIPaddress.getHumanReadableRepresentation())
		<< " ingressIP: " << *(ss.prevSiteIPaddress.getHumanReadableRepresentation()) << " - " << (int) ss.prevSiteIPaddressDistance
		<< endl;

		for(list<SubnetSiteNode *>::const_iterator i = ss.IPlist.begin(); i != ss.IPlist.end(); ++i)
		{
			out << "\t" << *(*i) << endl;
		}

		return out;
	}
	
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
	 * subnet we are expanding exists to some extent. It is then labelled as a "shadow" subnet.
	 */

    // Constructor/destructor
	SubnetSite();
	~SubnetSite();

	list<SubnetSiteNode*> *getSubnetIPList() { return &IPlist; }
	bool hasAlternativeSubnet();
	void clearIPlist();

	// Accessers (straightforward implementation)
	inline int getSubnetPositioningCost() { return this->spCost; }
	inline int getSubnetInferenceCost() { return this->siCost; }
	inline int getMergeAmount() { return this->mergeAmount; }
	inline InetAddress &getTargetAddress() { return this->targetIPaddress; }
	inline InetAddress &getPivotAddress() { return this->pivotIPaddress; }
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
    inline unsigned short getRefinementStatus() { return this->refinementStatus; }
    inline InetAddress &getRefinementContrapivot() { return this->refinementContrapivot; }
    inline unsigned char getRefinementShortestTTL() { return this->refinementTTL1; }
    inline unsigned char getRefinementGreatestTTL() { return this->refinementTTL2; }
    inline unsigned short getRefinementRouteSize() { return this->refinementRouteSize; }
    inline InetAddress *getRefinementRoute() { return this->refinementRoute; }
    
    // Setters for refinement data and prefix edition
    inline void setRefinementStatus(unsigned short rs) { this->refinementStatus = rs; }
    inline void setRefinementContrapivot(InetAddress &rc) { this->refinementContrapivot = rc; }
    inline void setRefinementShortestTTL(unsigned char rst) { this->refinementTTL1 = rst; }
    inline void setRefinementGreatestTTL(unsigned char rgt) { this->refinementTTL2 = rgt; }
    inline void setRefinementRouteSize(unsigned short rs) { this->refinementRouteSize = rs; }
    inline void setRefinementRoute(InetAddress *route) { this->refinementRoute = route; }
    inline void setInferredSubnetPrefixLength(unsigned char ispl) { this->inferredSubnetPrefix = ispl; }

    // Method to recompute the refinement status at certain points
    void recomputeRefinementStatus();
        
    // Method to know if a given InetAddress is within this subnet boundaries
    bool containsAddress(InetAddress i);
    
    // Method to know if a given InetAddress appears in the live interfaces of this subnet
    bool hasLiveInterface(InetAddress li);
    
    // Method to obtain a pivot address of this subnet after refinement
    InetAddress getRefinementPivot();
    
    // toString() method, only available for refined (odd/accurate) subnets, null otherwise 
    string refinedToString();
    
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
	unsigned short refinementRouteSize;
	InetAddress *refinementRoute;
};

#endif /* SUBNETSITE_H_ */

