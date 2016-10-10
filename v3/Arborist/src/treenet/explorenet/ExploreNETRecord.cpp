/*
 * ExploreNETRecord.cpp
 *
 *  Created on: Aug 29, 2016
 *      Author: grailet
 *
 * This file implements the ExploreNETRecord class (see this file for more details).
 */

#include <sstream>
using std::stringstream;
#include <iomanip> // For proper alignment with header lines (see TreeNETEnvironment.cpp)
using std::left;
using std::setw;

#include "ExploreNETRecord.h"

ExploreNETRecord::ExploreNETRecord(InetAddress target, 
                                   string inferred, 
                                   unsigned int SPC, 
                                   unsigned int SIC, 
                                   string alternative):
targetIP(target),
inferredSubnet(inferred),
subnetPositioningCost(SPC), 
subnetInferenceCost(SIC), 
alternativeSubnet(alternative)
{
}

ExploreNETRecord::~ExploreNETRecord()
{
}

string ExploreNETRecord::toString()
{
    stringstream recordStr;
    
    
    recordStr << left << setw(20) << targetIP;
    recordStr << left << setw(25) << inferredSubnet;
    recordStr << left << setw(6) << subnetPositioningCost;
    recordStr << left << setw(6) << subnetInferenceCost;
    recordStr << left << setw(18);
    if(!alternativeSubnet.empty() && alternativeSubnet.compare(inferredSubnet) != 0)
    {
        recordStr << alternativeSubnet;
    }
    else
    {
        recordStr << "None";
    }

    return recordStr.str();
}

bool ExploreNETRecord::compare(ExploreNETRecord *r1, ExploreNETRecord *r2)
{
    InetAddress ip1 = r1->getTargetIP();
    InetAddress ip2 = r2->getTargetIP();

    if (ip1 < ip2)
        return true;
    return false;
}
