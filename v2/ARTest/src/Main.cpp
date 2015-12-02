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

#include "toolbase/TreeNETEnvironment.h"
#include "toolbase/utils/TargetParser.h"
#include "toolbase/prescanning/NetworkPrescanner.h"
#include "toolbase/aliasresolution/AliasHintCollector.h"
#include "toolbase/aliasresolution/AliasResolver.h"

// Function to display usage of this program

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

    (*out) << "\t-i" << "\t" << "--input-file" << "\t\t\t" 
    << "input file (list of alias lists) path (separate IPs with blanks, lists with \\n)" << endl;
    
    (*out) << "\t-e" << "\t" << "--interface" << "\t\t\t" 
    << "interface name through which probing/response packets exit/enter (default is the first non-loopback IPv4 interface)" << endl;
    
    (*out) << "\t-u" << "\t" << "--probing-protocol" << "\t\t" << "probing protocol (ICMP, UDP or TCP; ICMP by default)" << endl;
    
    (*out) << "\t-m" << "\t" << "--attention-message" << "\t\t" << "probe attention message (default is \"NOT an ATTACK\")" << endl;
    
    (*out) << "\t-c" << "\t" << "--concurrency-nb-threads" << "\t" << "max number of threads (must be a positive integer)" << endl;
    
    (*out) << "\t-f" << "\t" << "--fix-flow-id" << "\t\t\t" << "use stable flow ID whenever possible [true|false] (default is true)" << endl;

    (*out) << "\t-s" << "\t" << "--third-opinion-prescan" << "\t\t" 
    << "performs a 3rd prescan (2 by default) with 4 times the initial timeout (default is false)" << endl;

    (*out) << "\t-w" << "\t" << "--probe-timeout-period" << "\t\t" 
    << "maximum milliseconds amount to wait for a probe reply (default is 2500)" << endl;
    
    (*out) << "\t-z" << "\t" << "--probe-regulating-period" << "\t" 
    << "minimum milliseconds amount to wait between two immediately consecutive probes (default is 50)" << endl;
    
    (*out) << "\t-d" << "\t" << "--probe-thread-delay" << "\t\t" 
    << "milliseconds amount to wait between the launch of two threads using probes (default is 250)" << endl;
    
    (*out) << "\t-a" << "\t" << "--amount-ip-ids" << "\t\t\t" << "(alias resol.) amount of collected IP IDs (default: 4)" << endl;
    (*out) << "\t-r" << "\t" << "--max-rollovers" << "\t\t\t" << "(alias resol.) maximum amount of IP ID counter rollover (default: 50)" << endl;
    (*out) << "\t-t" << "\t" << "--base-tolerance" << "\t\t" << "(alias resol.) tolerance for velocity overlap checking (default: 0.3)" << endl;
    (*out) << "\t-x" << "\t" << "--max-error" << "\t\t\t" << "(alias resol.) maximum error while rounding (default: 0.35)" << endl;
    
    (*out) << "\t-o" << "\t" << "--output-file" << "\t\t\t" << "name of the output file "
    << "(default is dd-mm-yyyy hh:mm:ss)" << endl;
    
    (*out) << "\t-g" << "\t" << "--debug" << "\t\t\t\t" << "enter debug mode" << endl;
    (*out) << "\t-v" << "\t" << "--version" << "\t\t\t" << "program version" << endl;
    (*out) << "\t-?" << "\t" << "--help" << "\t\t\t\t" << "help" << endl;
    (*out) << "TreeNET v2.0, Alias Resolution assessment version, written by Jean-Francois Grailet (5 November 2015)" << endl;
    (*out) << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;

    out->flush();
}

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default values (parameters)
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    unsigned short probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_ICMP;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@ulg.ac.be)");
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND); // 2s + 500 000 microseconds = 2,5s
    TimeVal probeRegulatingPeriod(0, 50000); // 0,05s
    TimeVal probeThreadDelay(0, 250000); // 0,25s
    unsigned short nbIPIDs = 4; // For Alias Resolution
    unsigned short maxRollovers = 50; // Idem
    double baseTolerance = 0.3; // Idem
    double maxError = 0.35; // Idem
    bool doubleProbe = false;
    bool useFixedFlowID = true;
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

    const char* const shortOpts = "e:u:m:i:c:f:s:w:z:d:a:r:t:x:o:gv?";
    const struct option longOpts[] = {
            {"interface", required_argument, 0, 'e'}, 
            {"probing-protocol", required_argument, 0, 'u'}, 
            {"attention-message", required_argument, 0, 'm'}, 
            {"input-file", required_argument, 0, 'i'}, 
            {"concurrency-nb-threads", required_argument, 0, 'c'}, 
            {"fix-flow-id", required_argument, 0, 'f'}, 
            {"third-opinion-prescan", required_argument, 0, 's'}, 
            {"probe-timeout-period", required_argument, 0, 'w'}, 
            {"probe-regulating-period", required_argument, 0, 'z'}, 
            {"probe-thread-delay", required_argument, 0, 'd'}, 
            {"amount-ip-ids", required_argument, 0, 'a'}, 
            {"max-rollovers", required_argument, 0, 'r'}, 
            {"base-tolerance", required_argument, 0, 't'}, 
            {"max-error", required_argument, 0, 'x'}, 
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
            case 'f':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("FALSE"))
                    useFixedFlowID = false;
                break;
            case 's':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("TRUE"))
                    thirdOpinionPrescan = true;
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
            case 'a':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb < 20)
                {
                    nbIPIDs = (unsigned short) gotNb;
                }
                break;
            case 'r':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb < 256)
                {
                    maxRollovers = (unsigned short) gotNb;
                }
                break;
            case 't':
                valDouble = StringUtils::string2double(optargSTR);
                if(valDouble >= 0.0)
                {
                    baseTolerance = valDouble;
                }
                break;
            case 'x':
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
                cout << "TreeNET v2.0, Alias Resolution assessment version, written by Jean-Francois Grailet (5 November 2015)" << endl;
                cout << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
                return 0;
            case '?':
                usage(&cout, string(argv[0]));
                return 0;
            default:
                usage(&cout, string(argv[0]));
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
            return 1;
        }
    }

    NetworkAddress LAN(localIPAddress, LANSubnetMask);
    
    // Initialization of the environment
    TreeNETEnvironment *env = new TreeNETEnvironment(&cout, 
                                                     probingProtocol, 
                                                     doubleProbe, 
                                                     useFixedFlowID, 
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
        
        // There is no input file
        if(inputFileContent.size() == 0)
        {
            cout << "Missing input file argument" << endl;
            usage(&cout, string(argv[0]));
            throw InvalidParameterException();
        }
        
        // Parses inputs and gets target lists
        TargetParser *parser = new TargetParser(env);
        parser->parseInputFile(inputFileContent);
        
        // Some welcome message
        cout << "Welcome to TreeNET (Alias Resolution assessment version)" << endl << endl;
        
        // Announces that it will ignore LAN.
        if(parser->targetsInLAN())
        {
            cout << "Some target IPs are located in the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored." << endl << endl;
        }
        
        list<InetAddress> targetsPrescanning = parser->getTargets();
        
        delete parser;
        
        // Stops if no target at all
        if(targetsPrescanning.size() == 0)
        {
            cout << "No target to probe." << endl;
            throw InvalidParameterException();
        }
        
        /*
         * NETWORK PRE-SCANNING
         *
         * Each interface from the suggested routers are probed to check that they are live IPs.
         * Like in TreeNET, there is by default a second opinion scan, and if explicitely asked, 
         * a third one. At each scan, the timeout delay is increased.
         *
         * It is worth noting that the unresponsive IPs will be updated in each router object 
         * only at the next step, juste before the live IPs are probed again during the alias 
         * resolution.
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
            else
            {
                cout << "All probed IPs were responsive." << endl << endl;
            }
        }
        else
        {
            cout << "All probed IPs were responsive." << endl << endl;
        }
        
        cout << "Prescanning ended." << endl << endl;
        
        delete prescanner;

        /*
         * ALIAS RESOLUTION
         *
         * Interfaces of a suggested router are probed once more to check if the A.R. part of 
         * TreeNET is able to infer the same result or finds something else.
         */ 
        
        cout << "Alias resolution..." << endl << endl;
        
        AliasHintCollector *ahc = new AliasHintCollector(env);
        AliasResolver *ar = new AliasResolver(env);
        
        list<Router*> *routers = env->getRouterList();
        for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
        {
            list<InetAddress> *exp = (*i)->getExpectedIPs();
            cout << "Checking [";
            bool first = true;
            for(list<InetAddress>::iterator j = exp->begin(); j != exp->end(); ++j)
            {
                if(first)
                    first = false;
                else
                    cout << ", ";
            
                cout << (*j);
            }
            cout << "]" << endl;
        
            // Alias resolution hints collection
            ahc->setIPsToProbe((*i));
            ahc->collect();
            
            // Proper alias resolution
            ar->resolve((*i));
        }
        delete ahc;
        delete ar;
        
        cout << endl;
        
        // Saves results
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
            
            newFileName = "Aliases " + timeStr;
        }
        
        env->outputRouterList(newFileName);
        cout << "Revisited routers have been saved in an output file ";
        cout << newFileName << endl;
        
        env->getIPTable()->outputDictionnary(newFileName + ".ip");
        cout << "IP dictionnary with alias resolution hints has been saved in an output file ";
        cout << newFileName + ".ip" << endl;
        
        env->outputPlotData(newFileName + ".plot");
        cout << "PDF/CDF Plot data has been saved in an output file ";
        cout << newFileName + ".plot" << endl;
    }
    catch(SocketException &e)
    {
        cout << "Unable to create sockets. Try running TreeNET as a privileged user (for ";
        cout << "example, try with sudo)." << endl;

        delete env;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "Use \"--help\" or \"-?\" parameter to reach help" << endl;
        delete env;
        return 1;
    }
    
    delete env;
    return 0;
}
