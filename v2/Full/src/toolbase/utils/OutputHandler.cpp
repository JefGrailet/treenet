/*
 * OutputHandler.cpp
 *
 *  Created on: Oct 12, 2014
 *      Author: grailet
 *
 * Implements the class defined in OutputHandler.h.
 */

#include "OutputHandler.h"
// #include "../explorenet/ExploreNETRunnable.h"

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iomanip>
using std::left;
using std::right;
using std::setw;

OutputHandler::OutputHandler(ostream *o, bool rhn, bool shAlt):
out(o),
resolveHostNames(rhn),
showAlternatives(shAlt)
{
}

OutputHandler::~OutputHandler()
{
}

void OutputHandler::usage(string programName)
{
    std::size_t lastSlashPos = programName.rfind('/');
    (*out) << "Usage: " << endl;
    if(lastSlashPos == string::npos)
    {
        (*out) << programName << endl;
    }
    else
    {
        (*out) << programName.substr(lastSlashPos+1) << endl;
    }
    (*out) << "\t-t" << "\t" << "--target" << "\t\t\t" << "target IP address(es) or host names (separate targets with commas)" << endl;
    
    (*out) << "\t-i" << "\t" << "--input-file" << "\t\t\t" << "input file (list of targets) path (separate targets with \\n)" << endl;
    
    (*out) << "\t-e" << "\t" << "--interface" << "\t\t\t" 
    << "interface name through which probing/response packets exit/enter (default is the first non-loopback IPv4 interface)" << endl;
    
    (*out) << "\t-h" << "\t" << "--middle-hop" << "\t\t\t" 
    << "middle hop distance (TTL) (default is 1)" << endl;
    
    (*out) << "\t-u" << "\t" << "--probing-protocol" << "\t\t" << "probing protocol (ICMP, UDP or TCP; ICMP by default)" << endl;
    
    (*out) << "\t-k" << "\t" << "--double-probe" << "\t\t\t" << "using two consecutive probes for each probe (default is false)" << endl;
    
    (*out) << "\t-m" << "\t" << "--attention-message" << "\t\t" << "probe attention message (default is \"NOT an ATTACK\")" << endl;
    
    (*out) << "\t-n" << "\t" << "--use-network-address" << "\t\t" 
    << "assume network address, i.e., ending all zeros, as an assignable IP address [true|false] (default is true)" << endl;
    
    (*out) << "\t-c" << "\t" << "--concurrency-nb-threads" << "\t" << "max number of threads (must be a positive integer)" << endl;
    
    (*out) << "\t-r" << "\t" << "--resolve-host-names" << "\t\t" 
    << "use reverse DNS to resolve host names and show them along with subnet IP addresses [true|false] (default is false)" << endl;
    
    (*out) << "\t-l" << "\t" << "--explore-lan-explicitly" << "\t" 
    << "discover the IP addresses in the Local Area Network (LAN) [true|false] (default is false)" << endl;

    (*out) << "\t-f" << "\t" << "--fix-flow-id" << "\t\t\t" << "use stable flow ID whenever possible [true|false] (default is true)" << endl;
    
    (*out) << "\t-p" << "\t" << "--prescan-expansion" << "\t\t" 
    << "probe encompassing /20 block for targets which consist of single IPs/small IP blocks [true|false] (default is false)" << endl;
    
    (*out) << "\t-s" << "\t" << "--third-opinion-prescan" << "\t\t" 
    << "performs a 3rd prescan (2 by default) with 4 times the initial timeout [true|false] (default is false)" << endl;
    
    (*out) << "\t-a" << "\t" << "--show-alternatives" << "\t\t" << "show alternative subnets if they exist [true|false] (default is true)" << endl;
    
    (*out) << "\t-w" << "\t" << "--probe-timeout-period" << "\t\t" 
    << "maximum milliseconds amount to wait for a probe reply (default is 2500)" << endl;
    
    (*out) << "\t-z" << "\t" << "--probe-regulating-period" << "\t" 
    << "minimum milliseconds amount to wait between two immediately consecutive probes (default is 50)" << endl;
    
    (*out) << "\t-d" << "\t" << "--probe-thread-delay" << "\t\t" 
    << "milliseconds amount to wait between the launch of two threads using probes (default is 250)" << endl;
    
    (*out) << "\t-j" << "\t" << "--amount-ip-ids" << "\t\t\t" << "(alias resol.) amount of collected IP IDs (default: 4)" << endl;
    (*out) << "\t-x" << "\t" << "--max-rollovers" << "\t\t\t" << "(alias resol.) maximum amount of IP ID counter rollover (default: 10)" << endl;
    (*out) << "\t-b" << "\t" << "--base-tolerance" << "\t\t" << "(alias resol.) tolerance for velocity overlap checking (default: 0.2)" << endl;
    (*out) << "\t-y" << "\t" << "--max-error" << "\t\t\t" << "(alias resol.) maximum error while rounding (default: 0.35)" << endl;
    
    (*out) << "\t-o" << "\t" << "--output-file" << "\t\t\t" << "label of the output files "
    << "(default is dd-mm-yyyy hh:mm:ss)" << endl;
    
    (*out) << "\t-g" << "\t" << "--debug" << "\t\t\t\t" << "enter debug mode" << endl;
    (*out) << "\t-v" << "\t" << "--version" << "\t\t\t" << "program version" << endl;
    (*out) << "\t-?" << "\t" << "--help" << "\t\t\t\t" << "help" << endl;
    (*out) << "TreeNET v2.3, written by Jean-Francois Grailet (last updated: April 2016)" << endl;
    (*out) << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
    
    out->flush();
}

void OutputHandler::printHeaderLines()
{
    /*
     * Deprecated (1st then 2nd line):
     * << setw(5) << left << "SPC" << setw(5) << left << "SIC"
     * << setw(5) << left << "---" << setw(5) << left << "---"
     */

    // First line (titles of columns)
    (*out) << setw(17) << left << "Target IP" << setw(17) << left << "Subnet status" 
    << setw(5) << left << "MA" << setw(20) << left << "Network Number" << setw(6) << "Size" 
    << " : " << "  [ IP - Hop Distance List ]   [ IP Alias Pair ]";
    (*out) << endl;
    
    // Second line (dashes)
    (*out) << setw(17) << left << "---------------" << setw(17) << left << "---------------" 
    << setw(5) << left << "--" << setw(20) << left << "------------------" << setw(6) << "----" 
    << " : " << "  --------------------------   -----------------";
    (*out) << endl;
    
    out->flush();
}

void OutputHandler::printDiscoveredSite(SubnetSite *site)
{
    // N.B.: at this point in the code, site is assumed to be non null

    string targetIPstr = "*";
    InetAddress target = site->getTargetAddress();
    if (!target.isUnset())
        targetIPstr = *(target.getHumanReadableRepresentation());
    string subnetStatusStr = "N/A";
    // string pivotIPstr = "N/A";
    // string SPCstr = "N/A";
    // string SICstr = "N/A";
    string MAstr = "0";
    string NNstr = "N/A";
    string data = "";
    string sizeStr = "N/A";
    string tmpHostName;

    unsigned short siteStatus = site->getRefinementStatus();
    switch(siteStatus)
    {
        case SubnetSite::ACCURATE_SUBNET:
            subnetStatusStr = "ACCURATE";
            break;
        case SubnetSite::ODD_SUBNET:
            subnetStatusStr = "ODD";
            break;
        case SubnetSite::SHADOW_SUBNET:
            subnetStatusStr = "SHADOW";
            break;
        case SubnetSite::INCOMPLETE_SUBNET:
            subnetStatusStr = "INCOMPLETE";
            break;
        default:
            subnetStatusStr = "UNDEFINED";
            break;
    }

    // pivotIPstr = *(site->getPivotAddress().getHumanReadableRepresentation());
    // SPCstr = StringUtils::int2string(site->getSubnetPositioningCost());
    // SICstr = StringUtils::int2string(site->getSubnetInferenceCost());
    MAstr = StringUtils::int2string(site->getMergeAmount());
    NNstr = site->getInferredNetworkAddressString();

    data += "  [ ";
    list<SubnetSiteNode*> *IPlist = site->getSubnetIPList();
    unsigned char inferredPrefixLength = site->getInferredSubnetPrefixLength();
    int sizeMinus1 = site->getInferredSubnetSize() - 1;
    sizeStr = StringUtils::int2string(sizeMinus1 + 1);
    int count = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist->begin(); i != IPlist->end(); ++i)
    {
        if((*i)->prefix >= inferredPrefixLength)
        {
            data += *((*i)->ip.getHumanReadableRepresentation());
            if(resolveHostNames)
            {
                tmpHostName = *((*i)->ip.getHostName());
                data += string("(") + tmpHostName + string(")");
            }
            data += string(" - ") + StringUtils::Uchar2string((*i)->TTL);
            if(count < sizeMinus1)
            {
                data += ",";
                
                // Ensures one pair IP - hop distance is displayed per line
                data += "\n";
                data += "                                                                        ";
            }
            count++;
        }

    }
    data += " ]";

    InetAddress ingressIP = site->getIngressInterfaceAddress();
    InetAddress contraPivotIP = site->getInferredSubnetContraPivotAddress();

    data += "  [ ";
    if(!contraPivotIP.isUnset() && !ingressIP.isUnset())
    {
        data += *(contraPivotIP.getHumanReadableRepresentation());
        if(resolveHostNames)
        {
            tmpHostName = *(contraPivotIP.getHostName());
            data += string("(") + tmpHostName + string(")");
        }
        data += string(" | ");
        data += *(ingressIP.getHumanReadableRepresentation());
        if(resolveHostNames)
        {
            tmpHostName = *(ingressIP.getHostName());
            data += string("(") + tmpHostName + string(")");
        }
    }
    else
    {
        data += "N/A";
    }
    data += " ]";
    
    /**
     * Deprecated code; maybe useful later
     *
     * else if(result == ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE)
     * {
     *     pivotIPstr = *(site->getPivotAddress().getHumanReadableRepresentation());
     *     SPCstr = StringUtils::int2string(site->getSubnetPositioningCost());
     *     SICstr = StringUtils::int2string(site->getSubnetInferenceCost());
     *     NNstr = site->getInferredNetworkAddressString();
     *     data += "  Local Area Subnetwork (LAN), System information";
     * }
     */
    
    /*
     * Deprecated display:
     * << left << SPCstr << setw(5) << left << SICstr << setw(5)
     */
    
    (*out) << setw(17) << left << targetIPstr << setw(17) << left << subnetStatusStr << setw(5) 
    << left << MAstr << setw(20) << left << NNstr << setw(6) << left << sizeStr << " : " << data;
}

void OutputHandler::printAlternativeSite(SubnetSite *site)
{
    // N.B.: at this point in the code, site is assumed to have an alternative (see printSite())

    string NNstr = "N/A";
    string sizeStr = "N/A";
    string data = "";
    string tmpHostName;

    NNstr = site->getAlternativeNetworkAddressString();

    data += "  { ";
    list<SubnetSiteNode*> *IPlist = site->getSubnetIPList();
    unsigned char alternativePrefixLength = site->getAlternativeSubnetPrefixLength();
    int sizeMinus1 = site->getAlternativeSubnetSize() - 1;
    sizeStr = StringUtils::int2string(sizeMinus1 + 1);
    int count = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist->begin(); i != IPlist->end(); ++i)
    {
        if((*i)->prefix >= alternativePrefixLength)
        {
            data += *((*i)->ip.getHumanReadableRepresentation());
            if(resolveHostNames)
            {
                tmpHostName = *((*i)->ip.getHostName());
                data += string("(") + tmpHostName + string(")");
            }
            data += string(" - ") + StringUtils::Uchar2string((*i)->TTL);
            if(count < sizeMinus1)
            {
                data += ", ";
            }
            count++;
        }

    }
    data += " }";
    InetAddress ingressIP = site->getIngressInterfaceAddress();
    InetAddress contraPivotIP = site->getAlternativeSubnetContraPivotAddress();

    data += "  { ";
    if(!contraPivotIP.isUnset() && !ingressIP.isUnset())
    {
        data += *(contraPivotIP.getHumanReadableRepresentation());
        if(resolveHostNames)
        {
            tmpHostName = *(contraPivotIP.getHostName());
            data += string("(") + tmpHostName + string(")");
        }
        data += string(" | ");
        data += *(ingressIP.getHumanReadableRepresentation());
        if(resolveHostNames)
        {
            tmpHostName = *(ingressIP.getHostName());
            data += string("(") + tmpHostName + string(")");
        }
    }
    else
    {
        data += "N/A";
    }
    data += " }";

    (*out) << setw(46) << right << "Alternative              " << setw(20) << left 
    << NNstr << setw(6) << left << sizeStr << " ~ " << data;
}

void OutputHandler::printSite(SubnetSite *site)
{
    this->printDiscoveredSite(site);
    (*out) << endl;
    if(showAlternatives == true)
    {
        if(site->hasAlternativeSubnet())
        {
            this->printAlternativeSite(site);
            (*out) << endl;
        }
    }
    out->flush();
}
