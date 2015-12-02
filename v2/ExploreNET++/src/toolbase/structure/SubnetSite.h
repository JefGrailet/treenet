/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (starting from October 2014) to improve coding style, study the code 
 * and implement new fields and methods for the needs of TreeNET. Adapted in November 2015 for the 
 * needs of ExploreNET++.
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
    enum Labels
    {
        NOT_PREPARED_YET, // Subnet exists but is not labelled yet
        ACCURATE_SUBNET, // Subnet contains its (single) contra-pivot
        INCOMPLETE_SUBNET, // Subnet contains only one IP or all IPs are at the same hop distance
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
    };

    // Constructor/destructor
    SubnetSite();
    ~SubnetSite();

    list<SubnetSiteNode*> *getSubnetIPList() { return &IPlist; }
    bool hasAlternativeSubnet();
    void clearIPlist();

    // Accessers (straightforward implementation)
    inline int getSubnetPositioningCost() { return this->spCost; }
    inline int getSubnetInferenceCost() { return this->siCost; }
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

    // Comparison methods for sorting purposes (based on lower bounds)
    static bool compare(SubnetSite *ss1, SubnetSite *ss2);
    
    // Method to insert a new interface in the IP list (made public for SubnetRefiner class)
    inline void insert(SubnetSiteNode *ssn) { this->IPlist.push_front(ssn); }
    
    // Method to call after inference is completed to compute the label
    void computeLabel();
    
    // Accessers to refined data (i.e. related to labelling process)
    inline unsigned short getLabel() { return this->label; }
    inline InetAddress &getContrapivot() { return this->contrapivot; }
    inline unsigned char getMinTTL() { return this->minTTL; }
    inline unsigned char getMaxTTL() { return this->maxTTL; }
    
    // Setters for refinement data and pivot/prefix edition
    inline void setLabel(unsigned short l) { this->label = l; }
    inline void setContrapivot(InetAddress &c) { this->contrapivot = c; }
    inline void setMinTTL(unsigned char mint) { this->minTTL = mint; }
    inline void setMaxTTL(unsigned char maxt) { this->maxTTL = maxt; }
    
    inline void setPivotAddress(InetAddress &pivot) { this->pivotIPaddress = pivot; }
    inline void setInferredSubnetPrefixLength(unsigned char ispl) { this->inferredSubnetPrefix = ispl; }
        
    // Method to know if a given InetAddress is within this subnet boundaries
    bool containsAddress(InetAddress i);
    
    // Method to know if a given InetAddress appears in the live interfaces of this subnet
    bool hasLiveInterface(InetAddress li);
    
    // toString() method, only available for refined subnets, null otherwise 
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

    InetAddress targetIPaddress;
    InetAddress pivotIPaddress; // SiteRecord.destination
    InetAddress prevSiteIPaddress; // Ingress interface
    unsigned char prevSiteIPaddressDistance;

    unsigned char inferredSubnetPrefix;
    unsigned char alternativeSubnetPrefix;
    SubnetSiteNode *contraPivotNode;
    
    // Fields dedicated to data related to the label (parts of refinement fields in TreeNET)
    unsigned short label;
    InetAddress contrapivot;
    unsigned short minTTL, maxTTL;
};

#endif /* SUBNETSITE_H_ */
