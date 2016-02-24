/*
 * OutputHandler.cpp
 *
 *  Created on: Jan 6, 2016
 *      Author: grailet
 *
 * Implements the class defined in OutputHandler.h.
 */

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iomanip>
using std::left;
using std::right;
using std::setw;

#include "OutputHandler.h"

OutputHandler::OutputHandler(ostream *out)
{
    this->out = out;
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
    
    (*out) << "\t-i" << "\t" << "--input-file" << "\t\t\t" << "names of the input file(s) (minus extension, i.e. .subnet or .ip)\n";
    (*out) << "\t\t\t\t\t\tIf multiple file names, separate them with commas. TreeNET will parse each\n";
    (*out) << "\t\t\t\t\t\tfile name followed by .subnet and will merge them together into a single set.\n";
    (*out) << "\t\t\t\t\t\tIf only one file, TreeNET will parse [file name].subnet and [file name].ip." << endl;
    
    (*out) << "\t-a" << "\t" << "--set-refinement" << "\t\t" 
    << "prevents collision and/or merge when parsing an already known subnet/IP (default is true)" << endl;
    
    (*out) << "\t-e" << "\t" << "--interface" << "\t\t\t" << "interface name through which probing/response packets exit/enter\n";
    (*out) << "\t\t\t\t\t\t(default is the first non-loopback IPv4 interface)" << endl;
    
    (*out) << "\t-u" << "\t" << "--probing-protocol" << "\t\t" << "probing protocol (ICMP, UDP or TCP; ICMP by default)" << endl;
    
    (*out) << "\t-m" << "\t" << "--attention-message" << "\t\t" << "probe attention message (default is \"NOT an ATTACK\")" << endl;

    (*out) << "\t-f" << "\t" << "--fix-flow-id" << "\t\t\t" << "use stable flow ID whenever possible [true|false] (default is true)" << endl;
    
    (*out) << "\t-w" << "\t" << "--probe-timeout-period" << "\t\t" 
    << "maximum milliseconds amount to wait for a probe reply (default is 2500)" << endl;
    
    (*out) << "\t-z" << "\t" << "--probe-regulating-period" << "\t" 
    << "minimum milliseconds amount to wait between two immediately consecutive probes (default is 50)" << endl;
    
    (*out) << "\t-t" << "\t" << "--concurrency-nb-threads" << "\t" 
    << "amount of threads being used while probing (default is 256)" << endl;
    
    (*out) << "\t-d" << "\t" << "--probe-thread-delay" << "\t\t" 
    << "milliseconds amount to wait between the launch of two threads using probes (default is 250)" << endl;
    
    (*out) << "\t-j" << "\t" << "--amount-ip-ids" << "\t\t\t" << "(alias resol.) amount of collected IP IDs (default: 4)\n";
    (*out) << "\t\t\t\t\t\tWARNING: this amount is also considered while parsing an IP dictionnary. If a line contains more\n";
    (*out) << "\t\t\t\t\t\tIP-IDs than specified, the exceeding will not be read. Reciprocally, line with less IP-IDs than\n";
    (*out) << "\t\t\t\t\t\tspecified will not be fully parsed." << endl;
    (*out) << "\t-x" << "\t" << "--max-rollovers" << "\t\t\t" << "(alias resol.) maximum amount of IP ID counter rollover (default: 50)" << endl;
    (*out) << "\t-c" << "\t" << "--base-tolerance" << "\t\t" << "(alias resol.) tolerance for velocity overlap checking (default: 0.3)" << endl;
    (*out) << "\t-y" << "\t" << "--max-error" << "\t\t\t" << "(alias resol.) maximum error while rounding (default: 0.35)" << endl;

    (*out) << "\t-l" << "\t" << "--label-output-files" << "\t\t" 
    << "label used for output files (default is dd-mm-yyyy hh:mm:ss)" << endl;
    
    (*out) << "\t-r" << "\t" << "--recomputation-mode" << "\t\t" << "modify the mode of re-computation\n";
    (*out) << "\t\t\t\t\t\tUse this option to properly merge separate datasets into a single one.\n";
    (*out) << "\t\t\t\t\t\tType 1 if you want to re-compute the alias resolution hints.\n";
    (*out) << "\t\t\t\t\t\tType 2 if you want to re-compute both the traceroutes and the alias resolution hints.\n";
    (*out) << "\t\t\t\t\t\tBy default, TreeNET Reader solely uses the data you provide." << endl;
    
    (*out) << "\t-s" << "\t" << "--statistics" << "\t\t\t" << "display various statistics about the inferred subnets" << endl;
    
    (*out) << "\t-n" << "\t" << "--neighborhoods" << "\t\t\t" << "display neighborhood analysis" << endl;
    
    (*out) << "\t-b" << "\t" << "--bipartite" << "\t\t\t" << "generate bipartite graph (given as an output file ";
    (*out) << "Bipartite [label])" << endl;
    
    (*out) << "\t-v" << "\t" << "--version" << "\t\t\t" << "program version" << endl;
    (*out) << "\t-?" << "\t" << "--help" << "\t\t\t\t" << "help" << endl;
    (*out) << "TreeNET Reader v2.1 (" << programName << ") was written by J.-F. Grailet (2016)\n";
    (*out) << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
}

void OutputHandler::printHeaderLines()
{
    // First line (titles of columns)
    (*out) << setw(17) << left << "Subnet status" << setw(20) << left << "Network Number" 
    << setw(6) << "Size" << " : " << "  [ IP - Hop Distance List ]";
    (*out) << endl;
    
    // Second line (dashes)
    (*out) << setw(17) << left << "---------------" << setw(20) << left << "------------------" 
    << setw(6) << "----" << " : " << "  --------------------------";
    (*out) << endl;
}

void OutputHandler::printSubnetSite(SubnetSite *site)
{
    // N.B.: at this point in the code, site is assumed to be non null
    string subnetStatusStr = "N/A";
    string NNstr = "N/A";
    string data = "";
    string sizeStr = "N/A";

    unsigned short siteStatus = site->getStatus();
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
        default:
            subnetStatusStr = "UNDEFINED";
            break;
    }

    NNstr = site->getInferredNetworkAddressString();

    data += "  [ ";
    list<SubnetSiteNode*> *IPlist = site->getSubnetIPList();
    int sizeMinus1 = site->getInferredSubnetSize() - 1;
    sizeStr = StringUtils::int2string(sizeMinus1 + 1);
    int count = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist->begin(); i != IPlist->end(); ++i)
    {
        data += *((*i)->ip.getHumanReadableRepresentation());
        data += string(" - ") + StringUtils::Uchar2string((*i)->TTL);
        
        if(count < sizeMinus1)
        {
            data += ",";
            
            // Ensures one pair IP - hop distance is displayed per line
            data += "\n";
            data += "                                                  ";
        }
        count++;
    }
    data += " ]";
    
    (*out) << setw(17) << left << subnetStatusStr << setw(20) << left << NNstr 
    << setw(6) << left << sizeStr << " : " << data;
    
    out->flush();
}
