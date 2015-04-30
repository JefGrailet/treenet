#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::exit;
#include <iomanip>
using std::setw;
using std::left;
#include <sstream>
using std::stringstream;
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ios;
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <list>
using std::list;
#include <algorithm> // For transform() function
#include <getopt.h> // For options parsing
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <ctime> // To obtain current time (for output file)
#include <sys/stat.h> // For CHMOD edition

#include "common/inet/InetAddress.h"
#include "common/inet/InetAddressException.h"
#include "common/inet/NetworkAddress.h"
#include "common/inet/NetworkAddressSet.h"
#include "common/inet/InetAddressSet.h"
#include "common/thread/Thread.h"
#include "common/thread/Runnable.h"
#include "common/thread/Mutex.h"
#include "common/thread/ConditionVariable.h"
#include "common/thread/MutexException.h"
#include "common/thread/TimedOutException.h"
#include "common/date/TimeVal.h"
#include "common/utils/StringUtils.h"
#include "common/random/PRNGenerator.h"
#include "common/random/Uniform.h"

#include "paristraceroute/ParisTracerouteTask.h"
#include "aliasresolution/AliasResolver.h"

#include "prober/structure/ProbeRecord.h"
#include "prober/icmp/DirectICMPProber.h"

#include "structure/SubnetSite.h"
#include "structure/SubnetSiteSet.h"
#include "structure/NetworkTree.h"

#include "bipartite/BipartiteGraph.h"

/**
 * Print out function to display usage of the program.
 *
 * @param ostream out           The output stream
 * @param string  programName   Name of the program (usually argv[0])
 */

void usage(ostream *out, string programName)
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
	
	(*out) << "\t-i" << "\t\t" << "--input-file" << "\t\t\t" << "input file(s) path (subnet list produced by TreeNET)" << endl;
	(*out) << "\t\t\t\t\t\t\tIf multiple files, separate them with commas." << endl;
	
	(*out) << "\t-e" << "\t\t" << "--interface" << "\t\t\t" << "interface name through which probing/response packets exit/enter" << endl;
	(*out) << "\t\t\t\t\t\t\t(default is the first non-loopback IPv4 interface)" << endl;
	
	(*out) << "\t-m" << "\t\t" << "--attention-message" << "\t\t" << "probe attention message (default is \"NOT an ATTACK\")" << endl;

	(*out) << "\t-f" << "\t\t" << "--fix-flow-id" << "\t\t\t" << "use stable flow ID whenever possible [true|false] (default is true)" << endl;
	
	(*out) << "\t-w" << "\t\t" << "--probe-timeout-period" << "\t\t" 
	<< "maximum milliseconds amount to wait for a probe reply (default is 2500)" << endl;
	
	(*out) << "\t-z" << "\t\t" << "--probe-regulating-period" << "\t" 
	<< "minimum milliseconds amount to wait between two immediately consecutive probes" << endl;
	(*out) << "\t\t\t\t\t\t\t(default is 50)" << endl;
	
	(*out) << "\t-t" << "\t\t" << "--concurrency-nb-threads" << "\t" 
	<< "amount of threads being used while probing (default is 256)" << endl;
	
	(*out) << "\t-l" << "\t\t" << "--label-output-files" << "\t\t" 
	<< "label used for output files (default is dd-mm-yyyy hh:mm:ss)" << endl;
	
	(*out) << "\t-a" << "\t\t" << "--set-refinement" << "\t\t" 
	<< "does merging/sorting at each insertion in the subnet set (default is true)" << endl;
	
	(*out) << "\t-s" << "\t\t" << "--statistics" << "\t\t\t" << "display various statistics about the inferred subnets" << endl;
	
	(*out) << "\t-r" << "\t\t" << "--recompute-routes" << "\t\t" << "recompute route to each parsed subnet (+ alias info)" << endl;
	
	(*out) << "\t-o" << "\t\t" << "--output-file" << "\t\t\t" << "write parsed subnet set in a new output "
	<< "[label]" << endl;
	
	(*out) << "\t-n" << "\t\t" << "--neighborhoods" << "\t\t\t" << "display neighborhood analysis" << endl;
	
	(*out) << "\t-b" << "\t\t" << "--bipartite" << "\t\t\t" << "generate bipartite graph (given as an output file ";
	(*out) << "Bipartite [label])" << endl;
	
	(*out) << "\t-v" << "\t\t" << "--version" << "\t\t\t" << "program version" << endl;
	(*out) << "\t-?" << "\t\t" << "--help" << "\t\t\t\t" << "help" << endl;
	(*out) << "TreeNET and TreeNET Reader were written by Jean-Francois Grailet (Academic year 2014 - 2015)" << endl;
	(*out) << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
	
	out->flush();
}

/**
 * Print out function which displays console headers to show the parsed subnets before building 
 * the network tree (such that the user can check if the parsing went well).
 *
 * @param ostream out  The output stream
 */

void printHeaderLines(ostream *out)
{
    // First line (titles of columns)
    (*out) << setw(17) << left << "Subnet status" << setw(20) << left << "Network Number" 
    << setw(6) << "Size" << " : " << "  [ IP - Hop Distance List ]";
	(*out) << endl;
	
	// Second line (dashes)
	(*out) << setw(17) << left << "---------------" << setw(20) << left << "------------------" 
	<< setw(6) << "----" << " : " << "  --------------------------";
	(*out) << endl;
	
	out->flush();
}

/**
 * Print out function to display the data from a parsed subnet (route aside).
 *
 * @param ostream out           The output stream
 * @param string  programName   Name of the program (usually argv[0])
 */

void printSubnetSite(ostream *out, SubnetSite *site)
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
}

// Main function

int main(int argc, char *argv[])
{
	InetAddress localIPAddress;
	string attentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@student.ulg.ac.be)");
	string inputFilePath;
	TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND);
	TimeVal probeRegulatingPeriod(0, 50000);
	unsigned short nbThreads = 256;
	string labelOutputFiles = "";
	bool useFixedFlowID = true;
	bool recomputeRoutes = false;
	bool newOutputFile = false;
	bool analyzeNeighborhoods = false;
	bool generateBipartite = false;
	bool computeStatistics = false;
	bool setRefinement = true;

	int opt = 0;
	int longIndex = 0;
	
	const char* const shortOpts = "i:e:m:f:w:z:t:l:a:sronbv?";
	const struct option longOpts[] = {
			{"input-file", required_argument, 0, 'i'},
			{"interface", required_argument, 0, 'e'},
			{"attention-message", required_argument, 0, 'm'},
			{"fix-flow-id", required_argument, 0, 'f'},
			{"probe-timeout-period", required_argument, 0, 'w'},
			{"probe-regulating-period", required_argument, 0, 'z'},
			{"concurrency-nb-threads", required_argument, 0, 't'},
			{"label-output-files", required_argument, 0, 'l'},
			{"set-refinement", required_argument, 0, 'a'},
			{"statistics", no_argument, NULL, 's'},
			{"recompute-routes", no_argument, NULL, 'r'},
			{"output-file", no_argument, NULL, 'o'},
			{"neighborhoods", no_argument, NULL, 'n'},
			{"bipartite", no_argument, NULL, 'b'},
			{"version", no_argument, NULL, 'v'},
		    {"help", no_argument, NULL, '?'}
	};

    // User arguments
	string optargSTR;
	unsigned long val;
	unsigned long sec;
	unsigned long microSec;
	while((opt = getopt_long(argc, argv, shortOpts, longOpts, &longIndex)) != -1)
	{
	    /*
	     * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
	     * Otherwise, it prevents the code from recognizing -v, -? and -g (because they need 
	     * no argument) and make it throw an exception... To avoid this, a second switch is used.
	     *
	     * (this is noteworthy, as this error is still present in ExploreNET v2.1)
	     */
	    
	    switch(opt)
	    {
	        case 's':
	        case 'r':
	        case 'o':
	        case 'n':
	        case 'b':
	        case 'v':
	        case '?':
	            break;
	        default:
		        optargSTR = string(optarg);
		        
		        /*
		        * For future readers: optarg is of type extern char*, and is defined in getopt.h.
		        * Therefore, you will not find the declaration of this variable in this file.
		        */
		        
		        break;
		}
		
		// Now we can actually treat the options.
		int gotNb = 0;
		switch(opt)
		{
		    case 'e':
			    try
			    {
				    localIPAddress = InetAddress::getLocalAddressByInterfaceName(optargSTR);
			    }
			    catch (InetAddressException &e)
			    {
				    cout << "Cannot obtain any IP address assigned to the interface \"" + optargSTR + "\"" << endl;
				    return 1;
			    }
			    break;
		    case 'm':
			    attentionMessage = optargSTR;
			    break;
            case 'i':
			    inputFilePath = optargSTR;
			    break;
		    case 'f':
			    std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
			    if(optargSTR == string("FALSE"))
				    useFixedFlowID = false;
			    break;
		    case 'w':
			    val = 1000 * StringUtils::string2Ulong(optargSTR);
			    if(val > 0)
			    {
				    sec = val / TimeVal::MICRO_SECONDS_LIMIT;
				    microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
				    timeoutPeriod.setTime(sec, microSec);
			    }
			    break;
		    case 'z':
			    val = 1000 * StringUtils::string2Ulong(optargSTR);
			    if(val > 0)
			    {
				    sec = val / TimeVal::MICRO_SECONDS_LIMIT;
				    microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
				    probeRegulatingPeriod.setTime(sec, microSec);
			    }
			    break;
			case 't':
			    gotNb = std::atoi(optargSTR.c_str());
			    if (gotNb > 0 && gotNb < 32767)
			    {
			        nbThreads = gotNb;
			    }
			    break;
			case 'l':
			    labelOutputFiles = optargSTR;
			    break;
			case 'a':
			    std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
			    if(optargSTR == string("FALSE"))
				    setRefinement = false;
			    break;
			case 's':
			    computeStatistics = true;
			    break;
			case 'r':
			    recomputeRoutes = true;
			    break;
			case 'o':
			    newOutputFile = true;
			    break;
			case 'n':
			    analyzeNeighborhoods = true;
			    break;
			case 'b':
			    generateBipartite = true;
			    break;
		    case 'v':
			    cout << string(argv[0]) << " (TreeNET Reader) v1.0 by J.-F. Grailet ";
			    cout << "(January - March 2015)" << endl;
			    return 0;
		    case '?':
			    usage(&cout, string(argv[0]));
			    return 0;
		    default:
			    usage(&cout, string(argv[0]));
			    return 1;
		}
	}

    // Determines local IP
	if(localIPAddress.isUnset() && recomputeRoutes)
	{
		try
		{
			localIPAddress = InetAddress::getFirstLocalAddress();
		}
		catch(InetAddressException &e)
		{
			cout << "Cannot obtain a valid local IP address for probing" << endl;
			return 1;
		}
	}
	
	// Set for maintaining subnets
    SubnetSiteSet *set = NULL;
	try
	{
	    // Opens input file (if any) and puts content inside a string
	    string inputs = "";
	    string warnings = ""; // To display unrecognized files later (after welcome message)
	    if(inputFilePath.size() > 0)
	    {
	        size_t pos = inputFilePath.find(',');
	        
	        // Single input file
	        if(pos == std::string::npos)
	        {
	            ifstream inFile;
		        inFile.open(inputFilePath.c_str());
		        if(inFile.is_open())
		        {
		            inputs.assign((std::istreambuf_iterator<char>(inFile)),
                                            (std::istreambuf_iterator<char>()));
                    
                    inFile.close();
		        }
		    }
		    // Multiple input files
		    else
		    {
	            std::stringstream ss(inputFilePath);
                std::string inputFileStr;
                while (std::getline(ss, inputFileStr, ','))
                {
                    string inputFileContent = "";
                    ifstream inFile;
                    inFile.open(inputFileStr.c_str());
		            if(inFile.is_open())
		            {
		                inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                                (std::istreambuf_iterator<char>()));
                        
                        inputs += inputFileContent;
                        
                        inFile.close();
		            }
		            else
		            {
		                warnings += "Warning: could not open file " + inputFileStr + "\n";
		            }
                }
		    }
	    }

        // Input file must contain something
        if(inputs.size() > 0)
        {
            // Some welcome message
            cout << "Welcome to TreeNET Reader" << endl << endl;
            
            if(warnings.size() > 0)
            {
                cout << warnings << endl;
            }
            
            set = new SubnetSiteSet();
        
            /*
             * SUBNET PARSING
             * 
             * The input files content, gathered in a single string, is parsed into separate
             * subnets which are inserted in the set if they are compatible (i.e. not redundant 
             * with the subnets that were already inserted).
             *
             * The format of the files must be precise such that the parsing can go well. In case 
             * of problem, error messages will be displayed in the console output.
             */
        
            cout << "Parsing subnets from the input..." << endl;
            std::stringstream ss(inputs);
            std::string targetStr;
            unsigned short nbLine = 0; // Counts the line while parsing
            bool ignoreTillBlankLine = false; // Ignores non-null lines until next blank line
            SubnetSite *temp = new SubnetSite(); // Temporar subnet
            while (std::getline(ss, targetStr, '\n'))
            {
                if(targetStr.size() == 0)
                {
                    // Parsing went well: insertion can occur
                    if(nbLine == 3 && !ignoreTillBlankLine)
                    {
                        temp->completeRefinedData();
                        
                        if(!setRefinement)
                        {
                            string subnetStr = temp->getInferredNetworkAddressString();
                            
                            set->addSiteNoRefinement(temp);
                            cout << "New subnet: " << subnetStr;
                            if (temp->isCredible())
                                cout << " (credible) ";
                            cout << endl;
                        }
                        else
                        {
                            unsigned short result = set->addSite(temp);
                            string subnetStr = temp->getInferredNetworkAddressString();
                            
                            if(result == SubnetSiteSet::NEW_SUBNET)
                            {
                                cout << "New subnet: " << subnetStr;
                                if (temp->isCredible())
                                    cout << " (credible) ";
                                cout << endl;
                            }
                            else if(result == SubnetSiteSet::KNOWN_SUBNET)
                            {
                                cout << "Known subnet: " << subnetStr << endl;
                                delete temp;
                            }
                            else if(result == SubnetSiteSet::SMALLER_SUBNET)
                            {
                                cout << "Merged with equivalent/larger subnet: " << subnetStr << endl;
                                delete temp;
                            }
                            else if(result == SubnetSiteSet::BIGGER_SUBNET)
                                cout << "Merged with smaller subnet(s): " << subnetStr << endl;
                        }
                    }
                    
                    // Resets everything
                    temp = new SubnetSite();
                    nbLine = 0;
                    ignoreTillBlankLine = false;
                }
                else
                {
                    if(ignoreTillBlankLine)
                        continue;
                    
                    // 0: first line is the CIDR notation for this subnet
                    if(nbLine == 0)
                    {
                        size_t pos = targetStr.find('/');
                        if(pos == std::string::npos)
                        {
                            ignoreTillBlankLine = true;
                            continue;
                        }
                        
                        std::string prefix = targetStr.substr(0, pos);
                        unsigned char prefixLength = (unsigned char) std::atoi(targetStr.substr(pos + 1).c_str());
                        
                        if((unsigned short) prefixLength > 32)
                        {
			                cout << "Malformed/Unrecognized subnet \"" + targetStr + "\"" << endl;
			                ignoreTillBlankLine = true;
                            continue;
                        }
                        
                        InetAddress subnetPrefix(0);
                        try
                        {
                            subnetPrefix.setInetAddress(prefix);
                        }
                        catch (InetAddressException &e)
		                {
			                cout << "Malformed/Unrecognized subnet \"" + targetStr + "\"" << endl;
			                ignoreTillBlankLine = true;
                            continue;
                        }
                        
                        temp->setInferredSubnetBaseIP(subnetPrefix);
                        temp->setInferredSubnetPrefixLength(prefixLength);
                        
                        nbLine++;
                    }
                    else if(nbLine == 1)
                    {
                        if(targetStr.compare("ACCURATE") == 0)
                            temp->setStatus(SubnetSite::ACCURATE_SUBNET);
                        else if(targetStr.compare("ODD") == 0)
                            temp->setStatus(SubnetSite::ODD_SUBNET);
                        else if(targetStr.compare("SHADOW") == 0)
                            temp->setStatus(SubnetSite::SHADOW_SUBNET);
                        else
                        {
                            string subnetStr = temp->getInferredNetworkAddressString();
                            cout << "Unrecognized status \"" + targetStr + "\" for subnet " << subnetStr << endl;
			                ignoreTillBlankLine = true;
                            continue;
                        }
                        
                        nbLine++;
                    }
                    else if(nbLine == 2)
                    {
                        size_t pos = targetStr.find(',');
                        
                        // Case where there is a single interface listed
                        if(pos == std::string::npos)
                        {
                            size_t pos2 = targetStr.find('-');
                            std::string IPStr = targetStr.substr(0, pos2 - 1);
                            std::string infoStr = targetStr.substr(pos2 + 1);
                            
                            InetAddress liveIP(0);
                            try
                            {
                                liveIP.setInetAddress(IPStr);
                            }
                            catch (InetAddressException &e)
		                    {
		                        string subnetStr = temp->getInferredNetworkAddressString();
			                    cout << "Malformed/Unrecognized interface \"" + targetStr;
			                    cout << "\" in subnet " << subnetStr << "." << endl << "As ";
			                    cout << "it is the only listed interface, this subnet will ";
			                    cout << "not be further parsed." << endl;
			                    ignoreTillBlankLine = true;
                                continue;
                            }
                            
                            // Parses TTL, probe token and IP identifier (or only TTL)
                            size_t pos3 = infoStr.find('[');
                            unsigned char TTL = 0;
                            unsigned int probeToken = 0;
                            unsigned short IPIdentifier = 0;
                            std::string hostName = "";
                            if(pos3 == std::string::npos)
                            {
                                TTL = (unsigned char) std::atoi(infoStr.c_str());
                            }
                            else
                            {
                                std::string TTLStr = infoStr.substr(0, pos3 - 1);
                                TTL = (unsigned char) std::atoi(TTLStr.c_str());
                                
                                std::string aliasInfo = infoStr.substr(pos3 + 2);
                                aliasInfo = aliasInfo.substr(0, aliasInfo.length() - 1);
                                size_t pos4 = aliasInfo.find('|');
                                if(pos4 != std::string::npos)
                                {
                                    std::string tokenStr = aliasInfo.substr(0, pos4);
                                    std::string IPIdStr = aliasInfo.substr(pos4 + 1);
                                    
                                    size_t pos5 = IPIdStr.find('|');
                                    if(pos5 != std::string::npos)
                                    {
                                        hostName = IPIdStr.substr(pos5 + 1);
                                        IPIdStr = IPIdStr.substr(0, pos5);
                                    }
                                    
                                    probeToken = (unsigned int) std::atoi(tokenStr.c_str());
                                    IPIdentifier = (unsigned short) std::atoi(IPIdStr.c_str());
                                }
                                else
                                {
                                    hostName = aliasInfo;
                                }
                            }
                            
                            if(probeToken > 0)
                            {
                                liveIP.setProbeToken(probeToken);
                                liveIP.setIPIdentifier(IPIdentifier);
                            }
                            
                            if(!hostName.empty())
                            {
                                liveIP.setStoredHostName(hostName);
                            }
                            
                            SubnetSiteNode *newNode = new SubnetSiteNode(liveIP, TTL);
                            temp->insert(newNode);
                            
                            nbLine++;
                            continue;
                        }
                        
                        // Case of multiple interfaces being listed (separated by ", ")
                        std::stringstream nodeStream(targetStr);
                        string nodeStr;
                        bool first = true;
                        bool atLeastOne = false;
                        while (std::getline(nodeStream, nodeStr, ','))
                        {
                            // Avoids the space after the coma (except for the first interface)
                            if(!first)
                                nodeStr = nodeStr.substr(1);
                            else
                                first = false;
                            
                            size_t pos2 = nodeStr.find('-');
                            std::string IPStr = nodeStr.substr(0, pos2 - 1);
                            std::string infoStr = nodeStr.substr(pos2 + 1);
                            
                            InetAddress liveIP(0);
                            try
                            {
                                liveIP.setInetAddress(IPStr);
                            }
                            catch (InetAddressException &e)
		                    {
		                        string subnetStr = temp->getInferredNetworkAddressString();
			                    cout << "Malformed/Unrecognized interface \"" + nodeStr;
			                    cout << "\" in subnet " << subnetStr << endl;
                                continue;
                            }

                            // Parses TTL, probe token, IP identifier and/or host name
                            size_t pos3 = infoStr.find('[');
                            unsigned char TTL = 0;
                            unsigned int probeToken = 0;
                            unsigned short IPIdentifier = 0;
                            std::string hostName = "";
                            if(pos3 == std::string::npos)
                            {
                                TTL = (unsigned char) std::atoi(infoStr.c_str());
                            }
                            else
                            {
                                std::string TTLStr = infoStr.substr(0, pos3 - 1);
                                TTL = (unsigned char) std::atoi(TTLStr.c_str());
                                
                                std::string aliasInfo = infoStr.substr(pos3 + 1);
                                aliasInfo = aliasInfo.substr(0, aliasInfo.length() - 1);
                                size_t pos4 = aliasInfo.find('|');
                                if(pos4 != std::string::npos)
                                {
                                    std::string tokenStr = aliasInfo.substr(0, pos4);
                                    std::string IPIdStr = aliasInfo.substr(pos4 + 1);
                                    
                                    size_t pos5 = IPIdStr.find('|');
                                    if(pos5 != std::string::npos)
                                    {
                                        hostName = IPIdStr.substr(pos5 + 1);
                                        IPIdStr = IPIdStr.substr(0, pos5);
                                    }
                                    
                                    probeToken = (unsigned int) std::atoi(tokenStr.c_str());
                                    IPIdentifier = (unsigned short) std::atoi(IPIdStr.c_str());
                                }
                                else
                                {
                                    hostName = aliasInfo;
                                }
                            }
                            
                            if(probeToken > 0)
                            {
                                liveIP.setProbeToken(probeToken);
                                liveIP.setIPIdentifier(IPIdentifier);
                            }
                            
                            if(!hostName.empty())
                            {
                                liveIP.setStoredHostName(hostName);
                            }
                            
                            // Finally inserts node
                            SubnetSiteNode *newNode = new SubnetSiteNode(liveIP, TTL);
                            temp->insert(newNode);
                            atLeastOne = true;
                        }
                        
                        if(!atLeastOne)
                        {
		                    cout << "No correct interface was listed. This subnet will not ";
		                    cout << "be further parsed." << endl;
		                    ignoreTillBlankLine = true;
                            continue;
                        }
                    
                        nbLine++;
                    }
                    else if(nbLine == 3)
                    {
                        list<InetAddress> routeLs;
                        size_t pos = targetStr.find(',');
                        
                        // Case where there is a single interface listed
                        if(pos == std::string::npos)
                        {
                            // Parses probe token and IP identifier if any
                            size_t pos2 = targetStr.find('[');
                            std::string IPStr = targetStr;
                            std::string infoStr = "";
                            unsigned int probeToken = 0;
                            unsigned short IPIdentifier = 0;
                            std::string hostName = "";
                            if(pos2 != std::string::npos)
                            {
                                IPStr = targetStr.substr(0, pos2 - 1);
                                infoStr = targetStr.substr(pos2 + 1);
                                
                                size_t pos3 = infoStr.find('|');
                                if(pos3 != std::string::npos)
                                {
                                    infoStr = infoStr.substr(0, infoStr.length() - 1);
                                    std::string tokenStr = infoStr.substr(0, pos3);
                                    std::string IPIdStr = infoStr.substr(pos3 + 1);
                                    
                                    size_t pos4 = IPIdStr.find('|');
                                    if(pos4 != std::string::npos)
                                    {
                                        hostName = IPIdStr.substr(pos4 + 1);
                                        IPIdStr = IPIdStr.substr(0, pos4);
                                    }
                                    
                                    probeToken = (unsigned int) std::atoi(tokenStr.c_str());
                                    IPIdentifier = (unsigned short) std::atoi(IPIdStr.c_str());
                                }
                                else
                                {
                                    hostName = infoStr.substr(0, infoStr.length() - 1);
                                }
                            }
                        
                            InetAddress liveIP(0);
                            try
                            {
                                liveIP.setInetAddress(IPStr);
                            }
                            catch (InetAddressException &e)
		                    {
		                        string subnetStr = temp->getInferredNetworkAddressString();
			                    cout << "Malformed/Unrecognized interface \"" + targetStr;
			                    cout << "\" in the route of subnet " << subnetStr << ".";
			                    ignoreTillBlankLine = true;
                                continue;
                            }
                            
                            if(probeToken > 0)
                            {
                                liveIP.setProbeToken(probeToken);
                                liveIP.setIPIdentifier(IPIdentifier);
                            }
                            
                            if(!hostName.empty())
                            {
                                liveIP.setStoredHostName(hostName);
                            }
                            
                            InetAddress *route = new InetAddress[1];
                            route[0] = liveIP;
                            temp->setRouteSize(1);
                            temp->setRoute(route);
                            
                            continue;
                        }
                        
                        // Case of multiple interfaces being listed (separated by ", ")
                        std::stringstream routeStream(targetStr);
                        string routeStr;
                        bool first = true;
                        bool routeOK = true;
                        while (std::getline(routeStream, routeStr, ','))
                        {
                            // Avoids the space after the coma (except for the first interface)
                            if(!first)
                                routeStr = routeStr.substr(1);
                            else
                                first = false;
                            
                            // Parses probe token and IP identifier if any
                            size_t pos2 = routeStr.find('[');
                            std::string IPStr = routeStr;
                            std::string infoStr = "";
                            unsigned int probeToken = 0;
                            unsigned short IPIdentifier = 0;
                            std::string hostName = "";
                            if(pos2 != std::string::npos)
                            {
                                IPStr = routeStr.substr(0, pos2 - 1);
                                infoStr = routeStr.substr(pos2 + 1);
                                
                                size_t pos3 = infoStr.find('|');
                                if(pos3 != std::string::npos)
                                {
                                    infoStr = infoStr.substr(0, infoStr.length() - 1);
                                    std::string tokenStr = infoStr.substr(0, pos3);
                                    std::string IPIdStr = infoStr.substr(pos3 + 1);
                                    
                                    size_t pos4 = IPIdStr.find('|');
                                    if(pos4 != std::string::npos)
                                    {
                                        hostName = IPIdStr.substr(pos4 + 1);
                                        IPIdStr = IPIdStr.substr(0, pos4);
                                    }
                                    
                                    probeToken = (unsigned int) std::atoi(tokenStr.c_str());
                                    IPIdentifier = (unsigned short) std::atoi(IPIdStr.c_str());
                                }
                                else
                                {
                                    hostName = infoStr.substr(0, infoStr.length() - 1);
                                }
                            }

                            InetAddress liveIP(0);
                            try
                            {
                                liveIP.setInetAddress(IPStr);
                            }
                            catch (InetAddressException &e)
		                    {
		                        string subnetStr = temp->getInferredNetworkAddressString();
			                    cout << "Malformed/Unrecognized interface \"" + routeStr;
			                    cout << "\" in route to subnet " << subnetStr << endl;
                                routeOK = false;
                                break;
                            }
                            
                            if(probeToken > 0)
                            {
                                liveIP.setProbeToken(probeToken);
                                liveIP.setIPIdentifier(IPIdentifier);
                            }
                            
                            if(!hostName.empty())
                            {
                                liveIP.setStoredHostName(hostName);
                            }
                            
                            routeLs.push_back(liveIP);
                        }
                        
                        if(!routeOK)
                        {
		                    cout << "Malformed route. This subnet will not be listed." << endl;
		                    ignoreTillBlankLine = true;
                            continue;
                        }
                        
                        unsigned short routeSize = (unsigned short) routeLs.size();
                        
                        InetAddress *route = new InetAddress[routeSize];
                        for(unsigned short i = 0; i < routeSize; i++)
                        {
                            route[i] = routeLs.front();
                            routeLs.pop_front();
                        }
                        
                        temp->setRouteSize(routeSize);
                        temp->setRoute(route);
                    }
                }
            }
            delete temp;
            
            size_t nbSubnets = set->getSubnetSiteList()->size();

            // Stops and throws an exception if no valid subnet
            if(nbSubnets == 0)
            {
                delete set;
                throw InvalidParameterException();
            }
            
            cout << "Parsing completed." << endl << endl;
            
	        /*
	         * PARIS TRACEROUTE
	         *
	         * If explicitely asked, we will re-compute the route to each subnet.
	         */
	         
	        std::list<SubnetSite*> *list = set->getSubnetSiteList();
            
            if(recomputeRoutes && list->size() > 0)
            {
                cout << "Recomputing route to each parsed subnet..." << endl << endl;
                
                // Copying list of subnets
                std::list<SubnetSite*> toSchedule;
                for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                {
                    toSchedule.push_back((*it));
                }
                
                // Size of the thread array
			    unsigned short sizeParisArray = 0;
			    unsigned short maxThreads = nbThreads;
			    if((unsigned long) toSchedule.size() > (unsigned long) maxThreads)
			        sizeParisArray = maxThreads;
			    else
			        sizeParisArray = (unsigned short) toSchedule.size();
			    
			    // Creates thread(s)
		        Thread **parisTh = new Thread*[sizeParisArray];
		        for(unsigned short i = 0; i < sizeParisArray; i++)
		            parisTh[i] = NULL;
			    
			    std::list<SubnetSite*> subnetsToDelete;
			    while(toSchedule.size() > 0)
			    {
			        unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
			        range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
			        range /= sizeParisArray;
			        
			        for(unsigned short i = 0; i < sizeParisArray && toSchedule.size() > 0; i++)
			        {
			            SubnetSite *curSubnet = toSchedule.front();
			            toSchedule.pop_front();
			            
			            unsigned short lowBound = (i * range);
			            unsigned short upBound = lowBound + range - 1;
			            parisTh[i] = new Thread(new ParisTracerouteTask(
		                             &cout,
		                             &subnetsToDelete,
		                             curSubnet,
		                             localIPAddress,
		                             attentionMessage,
		                             timeoutPeriod,
		                             probeRegulatingPeriod,
		                             DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowBound,
		                             DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upBound,
		                             DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
		                             DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
		            }

                    // Launches thread(s) then waits for completion
                    for(unsigned short i = 0; i < sizeParisArray; i++)
                    {
                        if(parisTh[i] != NULL)
                        {
                            parisTh[i]->start();
                        }
                    }
                    
                    for(unsigned short i = 0; i < sizeParisArray; i++)
                    {
                        if(parisTh[i] != NULL)
                        {
                            parisTh[i]->join();
                            delete parisTh[i];
                            parisTh[i] = NULL;
                        }
                    }
                }
                
                delete[] parisTh;
                
                // Removes subnets for which a route could not be recomputed
                if(subnetsToDelete.size() > 0)
                {
                    std::list<SubnetSite*>::iterator listBegin = subnetsToDelete.begin();
                    std::list<SubnetSite*>::iterator listEnd = subnetsToDelete.end();
                    for(std::list<SubnetSite*>::iterator i = listBegin; i != listEnd; ++i)
                    {
                        bool found = false;
                        for(std::list<SubnetSite*>::iterator j = list->begin(); j != list->end(); ++j)
                        {
                            if((*i) == (*j))
                            {
                                list->erase(j--);
                                found = true;
                                break;
                            }
                        }
                        
                        if(found)
                        {
                            SubnetSite *subnetToDelete = (*i);
                            subnetsToDelete.erase(i--);
                            delete subnetToDelete;
                        }
                    }
                }
            }
            
            cout << "Successfully parsed subnets:" << endl << endl;
	        
	        // Prints out headers
            printHeaderLines(&cout);
            
            // Outputting the parsed subnets (from the set)
            for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
            {
                printSubnetSite(&cout, *it);
                cout << endl;
            }
            cout << endl;
            
            // Get the current time for the name of output files (if any)
            time_t rawTime;
            struct tm *timeInfo;
            char buffer[80];

            time(&rawTime);
            timeInfo = localtime(&rawTime);

            // Name for the output file containing the list of subnets
            strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
            string timeStr(buffer);
            
            if(newOutputFile)
            {
                /*
                 * Writes a first save of the inferred subnets. If alias resolution works well, 
                 * the produced file will be overwritten. This is a security, in case something 
                 * went wrong during the construction of the tree.
                 */
                 
                string newFileName = timeStr;
                if(labelOutputFiles.length() > 0)
                    newFileName = labelOutputFiles;
                
                set->outputAsFile(newFileName);
                cout << "Parsed subnets (+ (new) routes) have been saved in an output file ";
                cout << newFileName << endl << endl;
            }
            
            /*
             * STATISTICS
             *
             * If asked by the user, this part of the program compute statistics about the
             * inferred subnets, which are:
             * -the total amount of IPs being covered by the subnets,
             * -the ratio live interaces/covered IPs,
             * -the amount of "credible" subnets (i.e. ACCURATE or ODD subnets with 1 
             *  contrapivot or a number representing less than 10% of the live interfaces),
             * -the ratio of such credible subnets in the full set,
             * -the total amount of neighborhoods,
             * -the total amount of neighborhoods with only subnets as children,
             * -the total amount of neighborhoods having complete linkage,
             * -the total amount of neighborhoods which labels appear in inferred subnets,
             * -the percentage of neighborhoods with only subnets as children,
             * -the percentage of neighborhoods having complete linkage,
             * -the percentage of neighborhoods which labels appear in inferred subnets.
             *
             * Despite the fact we will display these results at the end of the execution of this 
             * software, they need to be computed here because subnets that are valid for 
             * building the network tree will be removed from the set during the construction.
             */
            
            unsigned int totalCoveredIPs = 0;
            unsigned int totalLiveInterfaces = 0;
            double ratioLiveToCovered = 0.0;
            unsigned int totalSubnets = (unsigned int) list->size();
            unsigned int totalCredibleSubnets = 0;
            double ratioCredibleSubnets = 0.0;
            unsigned int totalNeighborhoods = 0;
            unsigned int totalNeighborhoodsLeaves = 0;
            unsigned int totalNeighborhoodsComplete = 0;
            unsigned int totalNeighborhoodsPartial = 0;
            unsigned int totalNeighborhoodsCoveredLabels = 0;
            double ratioNeighborhoodsLeaves = 0.0;
            double ratioNeighborhoodsComplete = 0.0;
            double ratioNeighborhoodsPartial = 0.0;
            double ratioNeighborhoodsCoveredLabels = 0.0;
            
            // Since the set will be emptied of valid subnets, statistics need to be computed now
            if(computeStatistics)
            {
                for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                {
                    if((*it) == NULL)
                        continue;
                
                    totalCoveredIPs += (*it)->getCapacity();
                    totalLiveInterfaces += (*it)->getTotalSize();
                
                    unsigned curStatus = (*it)->getStatus();
                    
                    if((curStatus == SubnetSite::ACCURATE_SUBNET || 
                        curStatus == SubnetSite::ODD_SUBNET) && (*it)->isCredible())
                    {
                        totalCredibleSubnets++;
                    }
                }
            }
            ratioLiveToCovered = (double) totalLiveInterfaces / (double) totalCoveredIPs;
            ratioCredibleSubnets = (double) totalCredibleSubnets / (double) totalSubnets;
            
            /*
             * SUBNET NEIGHBORHOOD INFERENCE
             *
             * Before inferring L2/L3 devices, we locate subnets regarding each other with a tree.
             */
             
            cout << "Building network tree..." << endl;
            
            unsigned short treeMaxDepth = set->getLongestRoute();
            set->sortByRoute();
            
            NetworkTree *tree = new NetworkTree(treeMaxDepth);
            
            SubnetSite *toInsert = set->getValidSubnet();
	        while(toInsert != NULL)
	        {
	            tree->insert(toInsert);
	            toInsert = set->getValidSubnet();
	        }
	        cout << "Building complete." << endl;
	        
	        tree->visit(&cout);
	        cout << endl;
	        
	        if(recomputeRoutes)
            {
	            /*
	             * ALIAS RESOLUTION
	             *
	             * Interfaces bordering a neighborhood are probed once more to attempt to gather 
	             * interfaces as routers.
	             */
	            
	            cout << "Alias resolution..." << endl << endl;
	            
	            std::list<InetAddress*> interfacesToProbe = tree->listInterfaces();

	            AliasResolver *ar = NULL;
	            ar = new AliasResolver(interfacesToProbe,
		                               localIPAddress, 
                                       attentionMessage, 
	                                   useFixedFlowID, 
                                       timeoutPeriod, 
                                       probeRegulatingPeriod, 
                                       DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                                       DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID,
                                       DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                                       DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ,
                                       nbThreads);
                ar->resolve();
                delete ar;
                
                tree->propagateRouterInfo();
                
                if(newOutputFile)
                {
                    string newFileName = timeStr;
                    if(labelOutputFiles.length() > 0)
                        newFileName = labelOutputFiles;
                
                    // Final save of the results, with everything needed.
                    tree->outputAsFile(newFileName);
                    cout << "Inferred subnets (+ routes and alias resolution hints) have ";
                    cout << "been saved in an output file " << newFileName << endl << endl;
                }
            }
            
            // We only infer routers now
            tree->visitAndInferRouters();
	        
	        // Statistics regarding the tree itself
	        if(computeStatistics)
	        {
	            unsigned int *stat = tree->getStatistics();
	            totalNeighborhoods = stat[0];
	            totalNeighborhoodsLeaves = stat[1];
	            totalNeighborhoodsComplete = stat[2];
	            totalNeighborhoodsPartial = stat[3];
	            totalNeighborhoodsCoveredLabels = stat[4];
	            
	            delete[] stat;
	            
	            ratioNeighborhoodsLeaves = (double) totalNeighborhoodsLeaves / (double) totalNeighborhoods;
	            ratioNeighborhoodsComplete = (double) totalNeighborhoodsComplete / (double) totalNeighborhoods;
	            ratioNeighborhoodsPartial = (double) totalNeighborhoodsPartial / (double) totalNeighborhoods;
	            ratioNeighborhoodsCoveredLabels = (double) totalNeighborhoodsCoveredLabels / (double) totalNeighborhoods;
	        }
	        
	        if(analyzeNeighborhoods)
	        {
	            tree->neighborhoods(&cout);
	        }
	        
	        if(generateBipartite)
	        {
	            cout << "Creating bipartite graph..." << endl << endl;
	            
	            // Generates bipartite graph
	            BipartiteGraph *graph = tree->toBipartite();
	            
	            // File name
                string filename = "Bipartite ";
                if(labelOutputFiles.length() > 0)
                    filename += labelOutputFiles;
                else
                    filename += timeStr;
                
                // Outputs the bipartite graph
                ofstream newFile;
                newFile.open(filename.c_str());
                newFile << graph->routersToString() << endl;
	            newFile << graph->subnetsToString() << endl;
	            newFile << graph->linksSRToString() << endl;
	            newFile << graph->linksRSToString() << endl;
                newFile.close();
                
                string newFilePath = "./" + filename;
                chmod(newFilePath.c_str(), 0766);
                
                cout << "Bipartite graph outputted as " << filename << endl << endl;
	            
	            delete graph;
	        }
	        
	        // Deletes tree
	        delete tree;
	        
	        // Deletes set
            delete set;
            
            // Now, we can display statistics if asked by the user.
            if(computeStatistics)
            {
                cout << "About the inferred subnets:" << endl;
                cout << "Total of covered IPs: " << totalCoveredIPs << endl;
                cout << "Total of responsive interfaces: " << totalLiveInterfaces << endl;
                cout << "Ratio responsive to covered IPs: " 
                << (ratioLiveToCovered * 100) << "%" << endl;
                
                cout << "Total of inferred subnets: " << totalSubnets << endl;
                cout << "Total of credible subnets: " << totalCredibleSubnets << endl;
                cout << "Ratio of credible subnets: " 
                << (ratioCredibleSubnets * 100) << "%" << endl;
                
                cout << "Total of neighborhoods: " << totalNeighborhoods << endl;
                
                cout << "Total of neighborhoods with only leaves: " << totalNeighborhoodsLeaves
                << " (" << (100 * ratioNeighborhoodsLeaves) << "%)" << endl;
                
                cout << "Total of neighborhoods with complete linkage: " 
                << totalNeighborhoodsComplete << " ("
                << (100 * ratioNeighborhoodsComplete) << "%)" << endl;
                
                cout << "Total of neighborhoods with partial linkage: " 
                << totalNeighborhoodsPartial << " ("
                << (100 * ratioNeighborhoodsPartial) << "%)" << endl;
                
                cout << "Total of neighborhoods which labels all appear in inferred subnets: " 
                << totalNeighborhoodsCoveredLabels << " ("
                << (100 * ratioNeighborhoodsCoveredLabels) << "%)" << endl;
            }
		}
		// No input
		else
		{
			cout << "Missing/empty input file" << endl;
			if(inputFilePath.length() > 0)
			    cout << "Input: " << inputFilePath << endl;
			cout << warnings << endl;
			usage(&cout, string(argv[0]));
			throw InvalidParameterException();
		}
	}
	catch(SocketException &e)
	{
	    cout << "Unable to create sockets to recompute routes and alias data. Try running ";
	    cout << "TreeNET Reader as a privileged user (for example, try with sudo)." << endl;
	    
	    if(set != NULL)
	        delete set;

	    return 1;
	}
	catch(InvalidParameterException &e)
	{
	    cout << "No valid subnet to work with." << endl;
		cout << "Use \"--help\" or \"-?\" parameter to reach help" << endl;
		return 1;
	}
	
	return 0;
}

