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
#include "toolbase/subnetinference/SubnetInferrer.h"
#include "toolbase/explorenet/ExploreNETRunnable.h"
#include "toolbase/ExploreNETEnvironment.h"

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Output handler object (with default values)
    OutputHandler *outputHandler = new OutputHandler(&cout, false, false);

    // Default values (parameters)
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    unsigned char inputStartTTL = (unsigned char) 1;
    unsigned short probingProtocol = ExploreNETEnvironment::PROBING_PROTOCOL_ICMP;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@ulg.ac.be)");
    bool useLowerBorderAsWell = true;
    bool exploreLANExplicitly = false;
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND); // 2s + 500 000 microseconds = 2,5s
    TimeVal probeRegulatingPeriod(0, 50000); // 0,05s
    TimeVal probeThreadDelay(0, 250000); // 0,25s
    bool doubleProbe = false;
    bool useFixedFlowID = true;
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

    const char* const shortOpts = "t:e:h:u:m:n:i:c:r:l:f:a:w:z:d:b:o:gv?";
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
            {"show-alternatives", required_argument, 0, 'a'}, 
            {"probe-timeout-period", required_argument, 0, 'w'}, 
            {"probe-regulating-period", required_argument, 0, 'z'}, 
            {"probe-thread-delay", required_argument, 0, 'd'}, 
            {"output-file", required_argument, 0, 'o'}, 
            {"debug", no_argument, NULL, 'g'}, 
            {"version", no_argument, NULL, 'v'}, 
            {"help", no_argument, NULL, '?'}
    };

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
                    probingProtocol = ExploreNETEnvironment::PROBING_PROTOCOL_UDP;
                else if(optargSTR == string("TCP"))
                    probingProtocol = ExploreNETEnvironment::PROBING_PROTOCOL_TCP;
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
            case 'o':
                outputFileName = optargSTR;
                break;
            case 'g':
                debugMode = true;
                break;
            case 'v':
                cout << "ExploreNET++, written by Jean-Francois Grailet (November 2015)" << endl;
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
    ExploreNETEnvironment *env = new ExploreNETEnvironment(&cout, 
                                                           inputStartTTL, 
                                                           probingProtocol, 
                                                           exploreLANExplicitly, 
                                                           useLowerBorderAsWell, 
                                                           doubleProbe, 
                                                           useFixedFlowID, 
                                                           localIPAddress, 
                                                           LAN, 
                                                           probeAttentionMessage, 
                                                           timeoutPeriod, 
                                                           probeRegulatingPeriod, 
                                                           probeThreadDelay, 
                                                           debugMode, 
                                                           nbThreads);

    // Gets direct access to the subnet set
    SubnetSiteSet *subnetSet = env->getSubnetSet();

    // Variable and structure which should be considered when catching some exception
    unsigned short sizeArray = 0;
    Thread **th = NULL;
    
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
        cout << "Welcome to ExploreNET++" << endl << endl;
        
        // Announces that it will ignore LAN.
        if(parser->targetsEncompassLAN())
        {
            cout << "Target IPs encompass the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored." << endl << endl;
        }
        
        /*
         * NETWORK SCANNING
         *
         * Given the set of (responsive) target addresses, TreeNET starts scanning the network 
         * by launching subnet discovery threads on each target.
         */
        
        // Gets the (responsive) target addresses
        std::list<InetAddress> targets = parser->getTargets();
        delete parser;
        
        // Stops if no target at all
        if(targets.size() == 0)
        {
            throw InvalidParameterException();
        }
        
        /*
         * Adapts the timeout value if the targets are close (as unsigned long int). When the 
         * gap between addresses is small, it is preferrable to increase the timeout period 
         * during the subnet inference in case it generated too much traffic at a particular 
         * network location.
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
             * SUBNET LABELLING
             *
             * Subnets found with previous threads are labelled in the same manner as in TreeNET.
             * However, there is no refinement here.
             */
            
            std::list<SubnetSite*> *list = subnetSet->getSubnetList();
            string newSubnets = "";
            for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
            {
                if((*it)->getLabel() != SubnetSite::NOT_PREPARED_YET)
                    continue;
                
                (*it)->computeLabel();
                string networkAddressStr = (*it)->getInferredNetworkAddressString();
                switch((*it)->getLabel())
                {
                    case SubnetSite::INCOMPLETE_SUBNET:
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
        }
        
        delete[] th;
        
        // Restores regular timeout (useful if future additions to this program)
        if(editedTimeout)
        {
            env->setTimeoutPeriod(timeoutPeriod);
        }
        
        cout << "Scanning completed." << endl << endl;

        // Prints out headers
        outputHandler->printHeaderLines();
        
        // Outputting results with the set.
        std::list<SubnetSite*> *list = subnetSet->getSubnetList();
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
            newFileName = "xnet++ " + outputFileName;
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
            
            newFileName = "xnet++ " + timeStr;
        }
        
        subnetSet->outputAsFile(newFileName);
        cout << endl << "Inferred subnets have been saved in an output file ";
        cout << newFileName << endl;
    }
    catch(SocketException &e)
    {
        cout << "Unable to create sockets. Try running ExploreNET++ as a privileged user (for ";
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
