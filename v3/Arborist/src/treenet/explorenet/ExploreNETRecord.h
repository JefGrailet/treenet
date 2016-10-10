/*
 * ExploreNETRecord.h
 *
 *  Created on: Aug 29, 2016
 *      Author: grailet
 *
 * This class is a simple structure to maintain the results of the subnet inference as performed 
 * by ExploreNET, that is, without the subnet refinement step. The goal is to be able to know, via 
 * a new option in TreeNET v3, what were the initial inference results before refinement. This can 
 * be used to both evaluate how much the refinement improved results, but it can also be helpful 
 * to understand difficult cases. Records should be written in a .xnet output file, with one 
 * record per line.
 *
 * It should be noted that a record saves only the most important details (target and the subnet 
 * that was inferred from it, SPC, SIC and alternative subnet if any) and could be extended to 
 * feature, for instance, the list of IPs per subnet - at the cost of having a much larger output 
 * file.
 */

#ifndef EXPLORENETRECORD_H_
#define EXPLORENETRECORD_H_

#include <string>
using std::string;

#include "../../common/inet/InetAddress.h"

class ExploreNETRecord
{
public:

    // Constructor and destructor
    ExploreNETRecord(InetAddress targetIP, 
                     string inferredSubnet, 
                     unsigned int subnetPositioningCost, 
                     unsigned int subnetInferenceCost, 
                     string alternativeSubnet);
    
    ~ExploreNETRecord();
    
    // Accesser
    inline InetAddress getTargetIP() { return this->targetIP; }
    
    // Methods to get a string equivalent and sort the records
    string toString();
    static bool compare(ExploreNETRecord *r1, ExploreNETRecord *r2);

private:
    
    // Private fields: all the relevant details
    InetAddress targetIP;
    string inferredSubnet;
    unsigned int subnetPositioningCost, subnetInferenceCost;
    string alternativeSubnet;
};

#endif /* EXPLORENETRECORD_H_ */
