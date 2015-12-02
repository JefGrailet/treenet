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
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ios;
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <list>
using std::list;
#include <algorithm> // For transform() function
#include <getopt.h> // For options parsing
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <ctime> // To obtain current time (for output file)
#include <unistd.h> // For usleep() function

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

#include "toolbase/utils/OutputHandler.h"
#include "toolbase/utils/TargetParser.h"
#include "toolbase/structure/SubnetSite.h"
#include "toolbase/structure/SubnetSiteSet.h"
#include "toolbase/structure/NetworkTree.h"
#include "toolbase/prescanning/NetworkPrescanner.h"
#include "toolbase/subnetinference/SubnetInferrer.h"
#include "toolbase/subnetrefinement/SubnetRefiner.h"
#include "toolbase/explorenet/ExploreNETRunnable.h"
#include "toolbase/paristraceroute/ParisTracerouteTask.h"
#include "toolbase/TreeNETEnvironment.h"

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Output handler object (with default values)
    OutputHandler *outputHandler = new OutputHandler(&cout, false, false);

    // Default values (parameters)
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    unsigned char inputStartTTL = (unsigned char) 1;
    unsigned short probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_ICMP;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@ulg.ac.be)");
    bool useLowerBorderAsWell = true;
    bool exploreLANExplicitly = false;
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND); // 2s + 500 000 microseconds = 2,5s
    TimeVal probeRegulatingPeriod(0, 50000); // 0,05s
    TimeVal probeThreadDelay(0, 250000); // 0,25s
    unsigned short nbIPIDs = 4; // For Alias Resolution
    unsigned short maxRollovers = 50; // Idem
    double baseTolerance = 0.3; // Idem
    double maxError = 0.35; // Idem
    bool doubleProbe = false;
    bool useFixedFlowID = true;
    bool prescanExpand = false;
    bool thirdOpinionPrescan = false;
    bool debugMode = false;
    unsigned short nbThreads = 256;
    
    // Values related to input/output (files)
    string targetsStr;
    string inputFilePath;
    ifstream inFile;
    string outputFileName = "";

    // Starts dealing with input parameters
    int opt = 0;
    int longIndex = 0;

    const char* const shortOpts = "t:e:h:u:m:n:i:c:r:l:f:p:s:a:w:z:d:j:x:b:y:o:gv?";
    const struct option longOpts[] = {
            {"target", required_argument, 0, 't'}, 
            {"interface", required_argument, 0, 'e'}, 
            {"middle-hop", required_argument, 0, 'h'}, 
            {"probing-protocol", required_argument, 0, 'u'}, 
            {"attention-message", required_argument, 0, 'm'}, 
            {"use-network-address", required_argument, 0, 'n'}, 
            {"input-file", required_argument, 0, 'i'}, 
            {"concurrency-nb-threads", required_argument, 0, 'c'}, 
            {"resolve-host-names", required_argument, 0, 'r'}, 
            {"explore-lan-explicitly", required_argument, 0, 'l'}, 
            {"fix-flow-id", required_argument, 0, 'f'}, 
            {"prescan-expansion", required_argument, 0, 'p'}, 
            {"third-opinion-prescan", required_argument, 0, 's'}, 
            {"show-alternatives", required_argument, 0, 'a'}, 
            {"probe-timeout-period", required_argument, 0, 'w'}, 
            {"probe-regulating-period", required_argument, 0, 'z'}, 
            {"probe-thread-delay", required_argument, 0, 'd'}, 
            {"amount-ip-ids", required_argument, 0, 'j'}, 
            {"max-rollovers", required_argument, 0, 'x'}, 
            {"base-tolerance", required_argument, 0, 'b'}, 
            {"max-error", required_argument, 0, 'y'}, 
            {"output-file", required_argument, 0, 'o'}, 
            {"debug", no_argument, NULL, 'g'}, 
            {"version", no_argument, NULL, 'v'}, 
            {"help", no_argument, NULL, '?'}
    };

    string optargSTR;
    unsigned long val;
    unsigned long sec;
    unsigned long microSec;
    double valDouble;
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
                targetsStr = optargSTR;
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
            case 'u':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("UDP"))
                    probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_UDP;
                else if(optargSTR == string("TCP"))
                    probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_TCP;
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
                    exploreLANExplicitly = true;
                break;
            case 'f':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("FALSE"))
                    useFixedFlowID = false;
                break;
            case 'p':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                    prescanExpand = true;
                break;
            case 's':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                    thirdOpinionPrescan = true;
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
            case 'd':
                val = 1000 * StringUtils::string2Ulong(optargSTR);
                if(val > 0)
                {
                    sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                    microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                    probeThreadDelay.setTime(sec, microSec);
                }
                break;
            case 'j':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 2 && gotNb <= 20)
                {
                    nbIPIDs = (unsigned short) gotNb;
                }
                break;
            case 'x':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb <= 256)
                {
                    maxRollovers = (unsigned short) gotNb;
                }
                break;
            case 'b':
                valDouble = StringUtils::string2double(optargSTR);
                if(valDouble >= 0.0)
                {
                    baseTolerance = valDouble;
                }
                break;
            case 'y':
                valDouble = StringUtils::string2double(optargSTR);
                if(valDouble >= 0.0 && valDouble < 0.5)
                {
                    maxError = valDouble;
                }
                break;
            case 'o':
                outputFileName = optargSTR;
                break;
            case 'g':
                debugMode = true;
                break;
            case 'v':
                cout << "TreeNET v2.0, written by Jean-Francois Grailet (v1.0: 2014-2015, v2.0: September - November 2015)" << endl;
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

    if(LANSubnetMask == 0)
    {
        try
        {
            LANSubnetMask = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localIPAddress);
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain subnet mask of the local area network (LAN)" << endl;
            delete outputHandler;
            return 1;
        }
    }

    NetworkAddress LAN(localIPAddress, LANSubnetMask);
    
    // Initialization of the environment
    TreeNETEnvironment *env = new TreeNETEnvironment(&cout, 
                                                     inputStartTTL, 
                                                     probingProtocol, 
                                                     exploreLANExplicitly, 
                                                     useLowerBorderAsWell, 
                                                     doubleProbe, 
                                                     useFixedFlowID, 
                                                     prescanExpand, 
                                                     localIPAddress, 
                                                     LAN, 
                                                     probeAttentionMessage, 
                                                     timeoutPeriod, 
                                                     probeRegulatingPeriod, 
                                                     probeThreadDelay, 
                                                     nbIPIDs, 
                                                     maxRollovers, 
                                                     baseTolerance, 
                                                     maxError, 
                                                     debugMode, 
                                                     nbThreads);
    
    // Gets direct access to the subnet set
    SubnetSiteSet *subnetSet = env->getSubnetSet();

    // Various variables/structures which should be considered when catching some exception
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
        
        // There is neither a destination parameter neither an input file
        if(targetsStr.size() == 0 && inputFileContent.size() == 0)
        {
            cout << "Missing destination IP address and/or input file argument" << endl;
            outputHandler->usage(string(argv[0]));
            throw InvalidParameterException();
        }
        
        // Parses inputs and gets target lists
        TargetParser *parser = new TargetParser(env);
        
        parser->parseCommandLine(targetsStr);
        parser->parseInputFile(inputFileContent);
        
        // Some welcome message
        cout << "Welcome to TreeNET" << endl << endl;
        
        // Announces that it will ignore LAN.
        if(parser->targetsEncompassLAN())
        {
            cout << "Target IPs encompass the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored." << endl << endl;
        }
        
        std::list<InetAddress> targetsPrescanning = parser->getTargetsPrescanning();

        // Stops if no target at all
        if(targetsPrescanning.size() == 0)
        {
            cout << "No target to probe." << endl;
            delete parser;
            throw InvalidParameterException();
        }
        
        /*
         * STEP I: NETWORK PRE-SCANNING
         *
         * Each address from the set of (re-ordered) target addresses are probed to check that 
         * they are live IPs.
         */
        
        NetworkPrescanner *prescanner = new NetworkPrescanner(env);
        prescanner->setTimeoutPeriod(env->getTimeoutPeriod());
        
        cout << "Prescanning with initial timeout..." << endl;
        prescanner->setTargets(targetsPrescanning);
        prescanner->probe();
        cout << endl;
        
        if(prescanner->hasUnresponsiveTargets())
        {
            TimeVal timeout2 = env->getTimeoutPeriod() * 2;
            cout << "Second opinion with twice the timeout (" << timeout2 << ")..." << endl;
            prescanner->setTimeoutPeriod(timeout2);
            prescanner->reloadUnresponsiveTargets();
            prescanner->probe();
            cout << endl;
            
            // Optional 3rd opinion prescan
            if(prescanner->hasUnresponsiveTargets() && thirdOpinionPrescan)
            {
                TimeVal timeout3 = env->getTimeoutPeriod() * 4;
                cout << "Third opinion with 4 times the timeout (" << timeout3 << ")..." << endl;
                prescanner->setTimeoutPeriod(timeout3);
                prescanner->reloadUnresponsiveTargets();
                prescanner->probe();
                cout << endl;
            }
        }
        else
        {
            cout << "All probed IPs were responsive." << endl << endl;
        }
        
        cout << "Prescanning ended." << endl << endl;
        
        delete prescanner;
        
        /*
         * STEP II: NETWORK SCANNING
         *
         * Given the set of (responsive) target addresses, TreeNET starts scanning the network 
         * by launching subnet discovery threads on each target. The inferred subnets are 
         * later merged together (when it is possible) to obtain a clean set of subnets where 
         * no subnet possibly contain another entry in the set.
         */
        
        // Gets the (responsive) target addresses
        std::list<InetAddress> targets = parser->getTargetsScanning();
        delete parser;
        
        /*
         * Adapts the timeout value if the targets are close (as unsigned long int). When the 
         * gap between addresses is small, it is preferrable to increase the timeout period 
         * during the subnet inference/refinement in case it generated too much traffic at a 
         * particular network location.
         */
        
        unsigned long smallestGap = 0;
        InetAddress previous("0.0.0.0");
        for(std::list<InetAddress>::iterator it = targets.begin(); it != targets.end(); ++it)
        {
            InetAddress cur = (*it);
            
            if(previous.getULongAddress() == 0)
            {
                previous = cur;
                continue;
            }
            
            unsigned long curGap = 0;
            if(cur.getULongAddress() > previous.getULongAddress())
                curGap = cur.getULongAddress() - previous.getULongAddress();
            else
                curGap = previous.getULongAddress() - cur.getULongAddress();
            
            if(smallestGap == 0 || curGap < smallestGap)
                smallestGap = curGap;
            
            previous = cur;
        }
        
        bool editedTimeout = false;
        if(smallestGap < 64)
        {
            env->setTimeoutPeriod(timeoutPeriod * 2);
            editedTimeout = true;
            cout << "Timeout adapted for network scanning: " << env->getTimeoutPeriod();
            cout << endl << endl;
        }
        
        // Size of threads vector
        unsigned long nbTargets = (unsigned int) targets.size();
        if(nbTargets > (unsigned long) nbThreads)
            sizeArray = nbThreads;
        else
            sizeArray = (unsigned short) nbTargets;
        
        // Creates thread(s)
        th = new Thread*[sizeArray];
        for(unsigned short i = 0; i < sizeArray; i++)
            th[i] = NULL;

        // Prepares subnet refiner (used during bypass)
        sr = new SubnetRefiner(env);
        
        cout << "Starting network scanning..." << endl << endl;
        
        while(nbTargets > 0)
        {
            unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
            range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
            range /= sizeArray;
            
            for(unsigned short i = 0; i < sizeArray; i++)
            {
                InetAddress curTarget(targets.front());
                targets.pop_front();
                
                unsigned short lowerBound = (i * range);
                unsigned short upperBound = lowerBound + range - 1;
                
                th[i] = new Thread(new ExploreNETRunnable(env, 
                                                          curTarget, 
                                                          DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowerBound, 
                                                          DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upperBound, 
                                                          DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                                          DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
            }

            // Launches thread(s) then waits for completion
            for(unsigned short i = 0; i < sizeArray; i++)
            {
                if(th[i] != NULL)
                {
                    th[i]->start();
                    Thread::invokeSleep(env->getProbeThreadDelay());
                }
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
            std::list<SubnetSite*> *list = subnetSet->getSubnetSiteList();
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
                
                SubnetSite *candidateForRefinement = subnetSet->getIncompleteSubnet();
                while(candidateForRefinement != NULL)
                {
                    sr->expand(candidateForRefinement);
                    subnetSet->addSite(candidateForRefinement);
                
                    candidateForRefinement = subnetSet->getIncompleteSubnet();
                }
                
                cout << "Back to scanning..." << endl << endl;
            }
        }
        
        delete[] th;
        
        // Restores regular timeout
        if(editedTimeout)
        {
            env->setTimeoutPeriod(timeoutPeriod);
        }
        
        cout << "Scanning completed." << endl << endl;
        
        /*
         * STEP III: SUBNET REFINEMENT (END)
         *
         * After the scanning (with bypass), subnets may still not contain all live 
         * interfaces in their list: a filling method helps to fix this issue by adding 
         * unlisted responsive interfaces that are within the boundaries of the subnet.
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
        std::list<SubnetSite*> *list = subnetSet->getSubnetSiteList();
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
             * Removes all shadow subnets, then puts them back. The motivation is to merge the 
             * subnets that have the same prefix length after shadow expansion, because it is 
             * rather frequent that several incomplete subnets lead to the same shadow subnet.
             *
             * It can also occur that a shadow subnet actually contains one or several outliers 
             * from another subnet. In that case, it should be merged with the larger subnet.
             */
            
            SubnetSite *shadow = subnetSet->getShadowSubnet();
            std::list<SubnetSite*> listShadows;
            while(shadow != NULL)
            {
                listShadows.push_back(shadow);
                shadow = subnetSet->getShadowSubnet();
            }
            
            std::list<SubnetSite*>::iterator listBegin = listShadows.begin();
            std::list<SubnetSite*>::iterator listEnd = listShadows.end();
            for(std::list<SubnetSite*>::iterator it = listBegin; it != listEnd; ++it)
            {
                subnetSet->addSite((*it));
            }
        }
        
        /*
         * STEP IV: PARIS TRACEROUTE
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
            if((unsigned long) toSchedule.size() > (unsigned long) nbThreads)
                sizeParisArray = nbThreads;
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

                    parisTh[i] = new Thread(new ParisTracerouteTask(env, 
                                                                    curSubnet, 
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
                        Thread::invokeSleep(env->getProbeThreadDelay());
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
        
        string newFileNameSubnet = "";
        string newFileNameIP = "";
        if(outputFileName.length() > 0)
        {
            newFileNameSubnet = outputFileName + ".subnet";
            newFileNameIP = outputFileName + ".ip";
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
            
            newFileNameSubnet = timeStr + ".subnet";
            newFileNameIP = timeStr + ".ip";
        }
        
        subnetSet->outputAsFile(newFileNameSubnet);
        cout << endl << "Inferred subnets (+ routes) have been saved in an output file ";
        cout << newFileNameSubnet << endl;
        
        /*
         * STEP V: SUBNET NEIGHBORHOOD INFERENCE
         *
         * Before inferring L2/L3 devices, we locate subnets regarding each other with a tree 
         * (hence the name "TreeNET"). See toolbase/structure/ to see how the construction of 
         * the tree works (and why it was designed this way).
         *
         * 19/10/2015: now, only the subnets with a complete route (i.e. no "0.0.0.0" appears in 
         * the route) are inserted at first. Then, for the remaining subnets, we look for the 
         * deepest match in the tree and complete the "holes" with IPs appearing in the routes to 
         * subnets which are found at the end of the branch where sits the deepest match. The goal 
         * is to avoid occurrences of 0.0.0.0 as much as possible (as they are not interpretable).
         */
        
        cout << endl << "Building network tree..." << endl;
         
        unsigned short treeMaxDepth = subnetSet->getLongestRoute();
        subnetSet->sortByRoute();
        
        NetworkTree *tree = new NetworkTree(treeMaxDepth);
        
        // Inserting subnets with a complete route first
        SubnetSite *toInsert = subnetSet->getValidSubnet();
        while(toInsert != NULL)
        {
            tree->insert(toInsert);
            toInsert = subnetSet->getValidSubnet();
        }
        
        cout << "Subnets with complete route inserted." << endl;
        cout << "Now repairing incomplete routes to insert remaining subnets..." << endl;
        
        // Then, subnets with an incomplete route after a repairment
        toInsert = subnetSet->getValidSubnet(false);
        while(toInsert != NULL)
        {
            tree->repairRoute(toInsert);
            tree->insert(toInsert);
            toInsert = subnetSet->getValidSubnet(false);
        }
        
        cout << "Building complete." << endl << endl;
        
        // Final save of the results.
        tree->outputSubnets(newFileNameSubnet);
        cout << "Inferred subnets (+ routes) have been saved in an output file ";
        cout << newFileNameSubnet << endl << endl;
        
        tree->visit(&cout);
        cout << endl;
        
        /*
         * STEP VI: ALIAS RESOLUTION
         *
         * Interfaces bordering a neighborhood are probed once more to attempt to gather 
         * interfaces as routers.
         */
        
        cout << "Alias resolution..." << endl << endl;
        
        // std::list<InetAddress*> interfacesToProbe = tree->listInterfaces();
        AliasHintCollector *ahc = new AliasHintCollector(env);
        tree->collectAliasResolutionHints(&cout, ahc);
        cout << endl;
        delete ahc;
        
        env->getIPTable()->outputDictionnary(newFileNameIP);
        cout << "IP dictionnary with alias resolution hints has been saved in an output file ";
        cout << newFileNameIP << endl << endl;

        // Internal node exploration is only done now.
        AliasResolver *ar = new AliasResolver(env);
        tree->internals(&cout, ar);
        
        delete ar;
        delete tree;
    }
    catch(SocketException &e)
    {
        cout << "Unable to create sockets. Try running TreeNET as a privileged user (for ";
        cout << "example, try with sudo)." << endl;
        
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
        delete env;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "Use \"--help\" or \"-?\" parameter to reach help" << endl;
        delete outputHandler;
        delete env;
        return 1;
    }
    
    delete outputHandler;
    delete env;
    return 0;
}
