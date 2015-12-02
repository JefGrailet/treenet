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

#include "prober/structure/ProbeRecord.h"
#include "prober/icmp/DirectICMPProber.h"

#include "toolbase/structure/SubnetSite.h"
#include "toolbase/structure/SubnetSiteSet.h"
#include "toolbase/structure/NetworkTree.h"
#include "toolbase/utils/TargetAddress.h"
#include "toolbase/subnetinference/SubnetInferrer.h"
#include "toolbase/subnetrefinement/SubnetRefiner.h"
#include "toolbase/explorenet/ExploreNETRunnable.h"
#include "toolbase/paristraceroute/ParisTracerouteTask.h"
#include "toolbase/aliasresolution/AliasResolver.h"

#include "OutputHandler.h"

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Output handler object (with default values)
    OutputHandler *outputHandler = new OutputHandler(&cout, false, false);

    // Default values
    InetAddress localIPAddress;
    unsigned char LANsubnetMask = 0;
    string targetIPstr;
    int inputStartTTL = 1;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@student.ulg.ac.be)");
    bool useLowerBorderAsWell = true;
    string inputFilePath;
    ifstream inFile;
    unsigned short nbThreads = 256; // Default
    bool exploreLANexplicitly = false;
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND);
    TimeVal probeRegulatingPeriod(0, 50000);
    TimeVal sampleRegulatingPausePeriod(0, 0);
    bool doubleProbe = false;
    bool useFixedFlowID = true;
    bool dbg = false;
    string outputFileName = "";

    int opt = 0;
    int longIndex = 0;

    const char* const shortOpts = "t:e:h:m:n:i:c:r:l:f:a:w:z:b:o:gv?";
    const struct option longOpts[] = {
            {"target", required_argument, 0, 't'},
            {"interface", required_argument, 0, 'e'},
            {"middle-hop", required_argument, 0, 'h'},
            {"attention-message", required_argument, 0, 'm'},
            {"use-network-address", required_argument, 0, 'n'},
            {"input-file", required_argument, 0, 'i'},
            {"concurrency-nb-threads", required_argument, 0, 'c'},
            {"resolve-host-names", required_argument, 0, 'r'},
            {"explore-lan-explicitly", required_argument, 0, 'l'},
            {"fix-flow-id", required_argument, 0, 'f'},
            {"show-alternatives", required_argument, 0, 'a'},
            {"probe-timeout-period", required_argument, 0, 'w'},
            {"probe-regulating-period", required_argument, 0, 'z'},
            {"output-file", required_argument, 0, 'o'},
            {"debug", no_argument, NULL, 'g'},
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
         * Otherwise, it prevents the code from recognizing flags like -v, -? or -g (because they 
         * require no argument) and make it throw an exception... To avoid this, a second switch 
         * is used.
         *
         * (this is noteworthy, as this error is still present in ExploreNET v2.1)
         */
        
        switch(opt)
        {
            case 'g':
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
            case 't':
                targetIPstr = optargSTR;
                break;
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
            case 'h':
                inputStartTTL = StringUtils::string2Uchar(optargSTR);
                break;
            case 'm':
                probeAttentionMessage = optargSTR;
                break;
            case 'n':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("FALSE"))
                    useLowerBorderAsWell = false;
                break;
            case 'i':
                inputFilePath = optargSTR;
                break;
            case 'c':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb < 32767)
                {
                    nbThreads = (unsigned short) gotNb;
                }
                break;
            case 'r':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                {
                    outputHandler->paramResolveHostNames(true);
                }
                break;
            case 'l':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                    exploreLANexplicitly = true;
                break;
            case 'f':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("FALSE"))
                    useFixedFlowID = false;
                break;
            case 'a':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                {
                    outputHandler->paramShowAlternatives(true);
                }
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
            case 'o':
                outputFileName = optargSTR;
                break;
            case 'g':
                dbg = true;
                break;
            case 'v':
                cout << "TreeNET v1.0, written by Jean-Francois Grailet (Academic year 2014-2015)" << endl;
                cout << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
                delete outputHandler;
                return 0;
            case '?':
                outputHandler->usage(string(argv[0]));
                delete outputHandler;
                return 0;
            default:
                outputHandler->usage(string(argv[0]));
                delete outputHandler;
                return 1;
        }
    }

    if(localIPAddress.isUnset())
    {
        try
        {
            localIPAddress = InetAddress::getFirstLocalAddress();
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain a valid local IP address for probing" << endl;
            delete outputHandler;
            return 1;
        }
    }

    if(LANsubnetMask == 0)
    {
        try
        {
            LANsubnetMask = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localIPAddress);
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain subnet mask of the local area network (LAN)" << endl;
            delete outputHandler;
            return 1;
        }
    }

    NetworkAddress lan(localIPAddress, LANsubnetMask);

    // Various variables/structures which should be considered when catching some exception
    SubnetSiteSet *set = NULL;
    unsigned short sizeArray = 0;
    Thread **th = NULL;
    SubnetRefiner *sr = NULL;
    
    try
    {
        // Opens input file (if any) and puts content inside a string
        string inputFileContent = "";
        if(inputFilePath.size() > 0)
        {
            inFile.open(inputFilePath.c_str());
            if(inFile.is_open())
            {
                inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                        (std::istreambuf_iterator<char>()));
                
                inFile.close();
            }
        }
    
        // Destination parameter is set OR input file is provided and readable
        if(targetIPstr.size() > 0 || inputFileContent.size() > 0)
        {
            // Computes the target address(es)
            std::list<TargetAddress> targets;
            
            // Targets are from a input string (given in console)
            if(targetIPstr.size() > 0)
            {
                std::stringstream ss(targetIPstr);
                std::string targetStr;
                while (std::getline(ss, targetStr, ','))
                {
                    // Ignore strings smaller than 6 chars
                    if (targetStr.size() < 6)
                        continue;
                
                    size_t pos = targetStr.find('/');
                    // Target is a single IP
                    if(pos == std::string::npos)
                    {
                        TargetAddress target;
                        target.startTTL = inputStartTTL;
                        try
                        {
                            target.address.setInetAddress(targetStr);
                            targets.push_back(target);
                        }
                        catch (InetAddressException &e)
                        {
                            cout << "Malformed/Unrecognized destination IP address or host name \"" + targetStr + "\"" << endl;
                            continue;
                        }
                    }
                    // Target is a whole subnet
                    else
                    {
                        std::string prefix = targetStr.substr(0, pos);
                        unsigned char prefixLength = (unsigned char) std::atoi(targetStr.substr(pos + 1).c_str());
                        try
                        {
                            InetAddress subnetPrefix(prefix);
                            NetworkAddress subnet(subnetPrefix, prefixLength);
                            
                            for(InetAddress cur = subnet.getLowerBorderAddress(); cur <= subnet.getUpperBorderAddress(); cur++)
                            {
                                TargetAddress target;
                                target.startTTL = inputStartTTL;
                                target.address = cur;
                                targets.push_back(target);
                            }
                        }
                        catch (InetAddressException &e)
                        {
                            cout << "Malformed/Unrecognized destination subnet \"" + targetStr + "\"" << endl;
                            continue;
                        }
                    }
                }
            }
            
            // Targets are from an input file
            if(inputFileContent.size() > 0)
            {
                std::stringstream ss(inputFileContent);
                std::string targetStr;
                while (std::getline(ss, targetStr, '\n'))
                {
                    // Ignore strings smaller than 6 chars
                    if (targetStr.size() < 6)
                        continue;
                
                    size_t pos = targetStr.find('/');
                    // Target is a single IP
                    if(pos == std::string::npos)
                    {
                        TargetAddress target;
                        target.startTTL = inputStartTTL;
                        try
                        {
                            target.address.setInetAddress(targetStr);
                            targets.push_back(target);
                        }
                        catch (InetAddressException &e)
                        {
                            cout << "Malformed/Unrecognized destination IP address or host name \"" + targetStr + "\"" << endl;
                            continue;
                        }
                    }
                    // Target is a whole subnet
                    else
                    {
                        std::string prefix = targetStr.substr(0, pos);
                        unsigned char prefixLength = (unsigned char) std::atoi(targetStr.substr(pos + 1).c_str());
                        try
                        {
                            InetAddress subnetPrefix(prefix);
                            NetworkAddress subnet(subnetPrefix, prefixLength);
                            
                            for(InetAddress cur = subnet.getLowerBorderAddress(); cur <= subnet.getUpperBorderAddress(); cur++)
                            {
                                TargetAddress target;
                                target.startTTL = inputStartTTL;
                                target.address = cur;
                                targets.push_back(target);
                            }
                        }
                        catch (InetAddressException &e)
                        {
                            cout << "Malformed/Unrecognized destination subnet \"" + targetStr + "\"" << endl;
                            continue;
                        }
                    }
                }
            }
            unsigned long nbTargets = (unsigned int) targets.size();

            // Stops and throws an exception if no valid target
            if(nbTargets == 0)
            {
                delete outputHandler;
                throw InvalidParameterException();
            }
            
            // Some welcome message
            cout << "Welcome to TreeNET" << endl << endl
                 << "Starting network scanning..." << endl << endl;
            
            /*
             * PART I: NETWORK SCANNING
             *
             * Given the set of target addresses, TreeNET starts scanning the network by launching 
             * subnet discovery threads on each target. The inferred subnets are later merged 
             * together (when it is possible) to obtain a clean set of subnets where no subnet 
             * possibly contain another entry in the set.
             */
            
            // Set for maintaining subnets
            set = new SubnetSiteSet();
            
            // Size of threads vector
            unsigned short maxThreads = nbThreads;
            if(nbTargets > (unsigned long) maxThreads)
            {
                sizeArray = maxThreads;
                
                /*
                 * TARGET REORDERING
                 *
                 * As targets are usually given in order, probing consecutive targets may be 
                 * inefficient as targetted interfaces may not be as responsive if multiple 
                 * threads probe them at the same time (because of the traffic generated at a 
                 * time), making inference results less complete and accurate. To avoid this, the 
                 * list of targets is re-organized to avoid consecutive IPs as much as possible.
                 */
               
                // Targets are put in an array
                TargetAddress array[nbTargets];
                unsigned short checked[nbTargets];
                for(unsigned long i = 0; i < nbTargets; i++)
                {
                    array[i] = targets.front();
                    checked[i] = 0;
                    targets.pop_front();
                }
                targets.clear();
                
                // A loop goes through array cells with a step of at least sizeArray
                unsigned long curIndex = 0;
                for(unsigned long i = 0; i < nbTargets; i++)
                {
                    targets.push_back(array[curIndex]);
                    checked[curIndex] = 1;
                    
                    // Looks for next unused target
                    if(i < nbTargets - 1)
                    {
                        curIndex += (unsigned long) sizeArray;
                        if(curIndex >= nbTargets)
                            curIndex -= nbTargets;
                        while(checked[curIndex] == 1)
                        {
                            curIndex++;
                            if(curIndex >= nbTargets)
                                curIndex -= nbTargets;
                        }
                    }
                }
            }
            else
            {
                sizeArray = (unsigned short) nbTargets;
            
                // In this case, we will just shuffle the list.
                std::vector<TargetAddress> targetsV(targets.size());
                std::copy(targets.begin(), targets.end(), targetsV.begin());
                std::random_shuffle(targetsV.begin(), targetsV.end());
                std::list<TargetAddress> shuffledTargets(targetsV.begin(), targetsV.end());
                targets = shuffledTargets;
            }
            
            // Creates thread(s)
            th = new Thread*[sizeArray];
            for(unsigned short i = 0; i < sizeArray; i++)
                th[i] = NULL;
            
            // Prepares subnet refiner (used during bypass)
            sr = new SubnetRefiner(&cout, 
                                   set,
                                   localIPAddress, 
                                   probeAttentionMessage, 
                                   useFixedFlowID, 
                                   timeoutPeriod, 
                                   probeRegulatingPeriod, 
                                   DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                                   DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID,
                                   DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                                   DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ,
                                   nbThreads);
            
            while(nbTargets > 0)
            {
                unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
                range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
                range /= sizeArray;
                
                for(unsigned short i = 0; i < sizeArray; i++)
                {
                    TargetAddress curTarget = targets.front();
                    targets.pop_front();
                    
                    unsigned short lowerBound = (i * range);
                    unsigned short upperBound = lowerBound + range - 1;
                    th[i] = new Thread(new ExploreNETRunnable(
                            set,
                            curTarget,
                            localIPAddress,
                            lan,
                            exploreLANexplicitly,
                            useLowerBorderAsWell,
                            probeAttentionMessage,
                            timeoutPeriod,
                            probeRegulatingPeriod,
                            doubleProbe,
                            useFixedFlowID,
                            DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowerBound,
                            DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upperBound,
                            DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                            DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ,
                            dbg));
                }

                // Launches thread(s) then waits for completion
                for(unsigned short i = 0; i < sizeArray; i++)
                {
                    if(th[i] != NULL)
                        th[i]->start();
                }
                
                for(unsigned short i = 0; i < sizeArray; i++)
                {
                    if(th[i] != NULL)
                    {
                        th[i]->join();
                        delete th[i];
                        th[i] = NULL;
                    }
                }
                
                cout << endl;
                
                // Updates number of targets for next salve of threads
                if(nbTargets > (unsigned long) sizeArray)
                {
                    nbTargets -= (unsigned long) sizeArray;
                    if(nbTargets < (unsigned long) sizeArray)
                        sizeArray = (unsigned short) nbTargets;
                }
                else
                    nbTargets = 0;
                
                /*
                 * BYPASS
                 *
                 * After probing a certain amount of target addresses, TreeNET periodically 
                 * stops the scanning in order to perform a first form of refinement: expansion.
                 * Indeed, ExploreNET tends to partition subnets when they do not feature a 
                 * certain amount of responsive interfaces. Expansion aims at correcting this 
                 * issue by relying on the Contra-Pivot notion (see SubnetRefiner.cpp/.h for more 
                 * details about this).
                 *
                 * Because expansion overgrowths subnet, performing this refinement may avoid 
                 * probing several addresses that are already inside the boundaries of the 
                 * refined subnets. Therefore, this should speed up the scanning which is the 
                 * slowest operation of TreeNET.
                 */
                
                // We first check which subnet should be refined
                std::list<SubnetSite*> *list = set->getSubnetSiteList();
                bool needsRefinement = false;
                string newSubnets = "";
                for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                {
                    if((*it)->getRefinementStatus() != SubnetSite::NOT_PREPARED_YET)
                        continue;
                    
                    (*it)->prepareForRefinement();
                    string networkAddressStr = (*it)->getInferredNetworkAddressString();
                    switch((*it)->getRefinementStatus())
                    {
                        case SubnetSite::INCOMPLETE_SUBNET:
                            needsRefinement = true;
                            newSubnets += networkAddressStr + ": incomplete subnet\n";
                            break;
                        case SubnetSite::ACCURATE_SUBNET:
                            newSubnets += networkAddressStr + ": accurate subnet\n";
                            break;
                        case SubnetSite::ODD_SUBNET:
                            newSubnets += networkAddressStr + ": odd subnet\n";
                            break;
                        default:
                            newSubnets += networkAddressStr + ": undefined subnet\n";
                            break;
                    }
                }
                
                if(!newSubnets.empty())
                {
                    cout << "New subnets found by the previous " << sizeArray << " threads:" << endl;
                    cout << newSubnets << endl;
                }
                else
                {
                    cout << "Previous " << sizeArray << " threads found no new subnet" << endl << endl;
                }
                
                // Performs refinement
                if(needsRefinement)
                {
                    cout << "Refining incomplete subnets..." << endl << endl;
                    
                    SubnetSite *candidateForRefinement = set->getIncompleteSubnet();
                    while(candidateForRefinement != NULL)
                    {
                        sr->expand(candidateForRefinement);
                        set->addSite(candidateForRefinement);
                    
                        candidateForRefinement = set->getIncompleteSubnet();
                    }
                    
                    cout << "Back to scanning..." << endl << endl;
                }
            }
            
            delete[] th;
            
            cout << "Scanning completed." << endl << endl;
            
            /*
             * PART II: SUBNET REFINEMENT (END)
             *
             * After the scanning (with bypass), subnets may still not contain all live 
             * interfaces in their list: a filling method helps to fix this issue by reprobing 
             * non-listed interfaces that are within the boundaries of the subnet.
             *
             * Regarding shadow subnets, if any, we also expand them so that their size 
             * (determined by their prefix) is the maximum size for these subnets to not collide 
             * with other inferred subnets. In other words, the code computes a lower bound on 
             * the prefix length, therefore an upper bound on the size of the subnet. This is the 
             * best we can predict for such cases.
             *
             * The code first lists the subnets to fill and checks if there are shadow subnets.
             * Then, the code proceeds to fill the subnets, using SubnetRefiner. There is no 
             * parallelization of the filling of several subnets, as there is already 
             * parallelization within the filling (with ProbesDispatcher, see subnetrefinement/). 
             * The very last step consists in expanding shadow subnets to their upper bound.
             */
            
            int nbShadows = 0;
            std::list<SubnetSite*> *list = set->getSubnetSiteList();
            std::list<SubnetSite*> toFill;
            for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
            {
                // Security for PlanetLab version (though this occurs rarely)
                if((*it) == NULL)
                    continue;
            
                unsigned short status = (*it)->getRefinementStatus();
                if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
                    toFill.push_back((*it));
                else if((*it)->getRefinementStatus() == SubnetSite::SHADOW_SUBNET)
                    nbShadows++;
            }
            
            // Filling
            if(toFill.size() > 0)
            {
                cout << "Starting refinement by filling..." << endl << endl;
                
                for(std::list<SubnetSite*>::iterator it = toFill.begin(); it != toFill.end(); ++it)
                {
                    // Security for PlanetLab version (though this should not occur)
                    if((*it) == NULL)
                        continue;
                    
                    unsigned short status = (*it)->getRefinementStatus();
                    
                    sr->fill(*it);
                    // (*it) != NULL: same reason as above (though, once again, unlikely)
                    if((*it) != NULL && status == SubnetSite::ACCURATE_SUBNET)
                        (*it)->recomputeRefinementStatus();
                }
                cout << endl;
            }
            
            // Shadow expansion (no parallelization here, because this operation is instantaneous)
            if(nbShadows > 0)
            {
                cout << "Expanding shadow subnets to the maximum..." << endl << endl;
                for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                {
                    if((*it)->getRefinementStatus() == SubnetSite::SHADOW_SUBNET)
                    {
                        sr->shadowExpand(*it);
                    }
                }
                
                /*
                 * Removes all Shadow subnets, then puts them back. The motivation is to merge the 
                 * subnets that have the same prefix length after shadow expansion, because it is 
                 * rather frequent that several incomplete subnets lead to the same shadow subnet.
                 */
                
                SubnetSite *shadow = set->getShadowSubnet();
                std::list<SubnetSite*> listShadows;
                while(shadow != NULL)
                {
                    listShadows.push_back(shadow);
                    shadow = set->getShadowSubnet();
                }
                
                std::list<SubnetSite*>::iterator listBegin = listShadows.begin();
                std::list<SubnetSite*>::iterator listEnd = listShadows.end();
                for(std::list<SubnetSite*>::iterator it = listBegin; it != listEnd; ++it)
                {
                    set->addSite((*it));
                }
            }
            
            /*
             * PART III: PARIS TRACEROUTE
             *
             * The next step consists in computing the route to each inferred subnet, no matter 
             * how it was classified during previous algorithmic steps.
             *
             * The route computation itself is base on the same ideas as Paris Traceroute, and is 
             * parallelized to quickly obtain all routes.
             */
        
            if(list->size() > 0)
            {
                cout << "Computing route to each accurate/odd/shadow subnet..." << endl << endl;
                
                // Lists subnets for which we would like a route
                std::list<SubnetSite*> toSchedule;
                for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                {
                    unsigned short status = (*it)->getRefinementStatus();
                    if(status == SubnetSite::ACCURATE_SUBNET || 
                       status == SubnetSite::SHADOW_SUBNET || 
                       status == SubnetSite::ODD_SUBNET)
                    {
                        toSchedule.push_back((*it));
                    }
                }
                
                // Size of the thread array
                unsigned short sizeParisArray = 0;
                if((unsigned long) toSchedule.size() > (unsigned long) maxThreads)
                    sizeParisArray = maxThreads;
                else
                    sizeParisArray = (unsigned short) toSchedule.size();
                
                // Creates thread(s)
                Thread **parisTh = new Thread*[sizeParisArray];
                for(unsigned short i = 0; i < sizeParisArray; i++)
                    parisTh[i] = NULL;
                
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
                                     curSubnet,
                                     localIPAddress,
                                     probeAttentionMessage,
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
            }
            
            delete sr;
            
            cout << "Finished computing routes. Subnets list will follow shortly." << endl << endl;
            
            // Prints out headers
            outputHandler->printHeaderLines();
            
            // Outputting results with the set.
            for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
            {
                outputHandler->printSite(*it);
            }
            
            /*
             * Writes a first save of the inferred subnets. If alias resolution works well, 
             * the produced file will be overwritten. This is a security, in case something 
             * went wrong during the construction of the tree.
             */
            
            string newFileName = "";
            if(outputFileName.length() > 0)
            {
                newFileName = outputFileName;
            }
            else
            {
                // Get the current time for the name of output file
                time_t rawTime;
                struct tm *timeInfo;
                char buffer[80];

                time(&rawTime);
                timeInfo = localtime(&rawTime);

                strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
                string timeStr(buffer);
                
                newFileName = timeStr;
            }
            
            set->outputAsFile(newFileName);
            cout << endl << "Inferred subnets (+ routes) have been saved in an output file " << newFileName << endl;
        
            cout << endl << "Building network tree..." << endl;
            
            /*
             * PART IV: SUBNET NEIGHBORHOOD INFERENCE
             *
             * Before inferring L2/L3 devices, we locate subnets regarding each other with a tree 
             * (hence the name "TreeNET"). See toolbase/structure/ to see how the construction of 
             * the tree works (and why it was designed this way).
             */
             
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
            
            /*
             * PART V: ALIAS RESOLUTION
             *
             * Interfaces bordering a neighborhood are probed once more to attempt to gather 
             * interfaces as routers.
             */
            
            cout << "Alias resolution..." << endl << endl;
            
            std::list<InetAddress*> interfacesToProbe = tree->listInterfaces();

            AliasResolver *ar = new AliasResolver(interfacesToProbe,
                                                  localIPAddress, 
                                                  probeAttentionMessage, 
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

            // Final save of the results, with everything needed.
            tree->outputAsFile(newFileName);
            cout << "Inferred subnets (+ routes and alias resolution hints) have ";
            cout << "been saved in an output file " << newFileName << endl << endl;

            // Neighborhood exploration is only done now.
            tree->neighborhoods(&cout);
            
            // Deletes tree
            delete tree;
        
            // Deletes set
            delete set;
        }
        // No input
        else
        {
            cout << "Missing destination IP address and/or input file argument" << endl;
            outputHandler->usage(string(argv[0]));
            throw InvalidParameterException();
        }
    }
    catch(SocketException &e)
    {
        cout << "Unable to create sockets. Try running TreeNET as a privileged user (for ";
        cout << "example, try with sudo)." << endl;
        
        if(set != NULL)
            delete set;
        
        if(th != NULL)
        {
            for(unsigned int i = 0; i < sizeArray; i++)
            {
                if(th[i] != NULL)
                    delete th[i];
            }
            delete[] th;
        }
        
        if(sr != NULL)
            delete sr;
        
        delete outputHandler;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "Use \"--help\" or \"-?\" parameter to reach help" << endl;
        delete outputHandler;
        return 1;
    }
    
    delete outputHandler;
    return 0;
}
