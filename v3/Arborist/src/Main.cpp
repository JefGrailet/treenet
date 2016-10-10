#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::exit;
#include <iomanip> // For the display of inferred/refined subnets
using std::left;
using std::setw;
#include <fstream>
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

#include "treenet/TreeNETEnvironment.h"
#include "treenet/utils/TargetParser.h"
#include "treenet/prescanning/NetworkPrescanner.h"
#include "treenet/explorenet/ExploreNETRunnable.h"
#include "treenet/subnetrefinement/SubnetRefiner.h"
#include "treenet/tree/NetworkTree.h"
#include "treenet/tree/growth/classic/ClassicGrower.h"
#include "treenet/tree/climbers/Robin.h"
#include "treenet/tree/climbers/Cuckoo.h"
#include "treenet/tree/climbers/Crow.h"

// Simple function to display usage.

void printInfo()
{
    cout << "Summary\n";
    cout << "=======\n";
    cout << "\n";
    cout << "TreeNET \"Arborist\" is a topology discovery tool designed to take advantage of\n";
    cout << "subnet inference to unveil, partially or completely, the underlying topology\n";
    cout << "of a target network. To do so, it uses ExploreNET (a subnet discovery tool\n";
    cout << "designed and developed by Ph. D. Mehmet Engin Tozal) to discover subnets\n";
    cout << "within the target network and uses (Paris) traceroute to obtain a route to\n";
    cout << "each of them. Both routes and subnets are then used to build a tree-like\n";
    cout << "structure (hence the name \"TreeNET\", a mix between \"tree\" and \"ExploreNET\")\n";
    cout << "that models the observed network, with the internal nodes modeling\n";
    cout << "\"neighborhoods\", that is, a set of machines all surrounded by subnets that are\n";
    cout << "located at at most one hop from each other. The collected data along the\n";
    cout << "interpretation of the tree allows to go a step further and discover routers\n";
    cout << "through alias resolution.\n";
    cout << "\n";
    cout << "From an algorithmic point of view, TreeNET consists in a sequence of different\n";
    cout << "steps that are either active (i.e., with probing), either passive\n";
    cout << "(interpretation of the data). While other resources provide much more details\n";
    cout << "about how TreeNET works, a \"big picture\" of the different steps is provided\n";
    cout << "below. It is worth noting that TreeNET is currently only available for IPv4,\n";
    cout << "as moving to IPv6 will require a working IPv6 subnet inference algorithm.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "N.B.: as Arborist is built on the original TreeNET, it is simply denoted as\n";
    cout << "\"TreeNET\" in the next lines.\n";
    cout << "\n";
    cout << "0) Launch and target selection\n";
    cout << "------------------------------\n";
    cout << "\n";
    cout << "TreeNET parses its main argument to get a list of all the target IPs it should\n";
    cout << "consider in the next steps.\n";
    cout << "\n";
    cout << "1) Network pre-scanning\n";
    cout << "-----------------------\n";
    cout << "\n";
    cout << "Each target IP is probed once to evaluate its liveness. Unresponsive IPs are\n";
    cout << "probed a second time with twice the initial timeout as a second opinion (note:\n";
    cout << "a third opinion can be triggered via a flag). Only IPs that were responsive\n";
    cout << "during this step will be probed again during the next steps, avoiding useless\n";
    cout << "probing work. Several target IPs are probed at the same time, via\n";
    cout << "multi-threading.\n";
    cout << "\n";
    cout << "2) Network scanning\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "Responsive target IPs are analyzed with ExploreNET, with multi-threading to\n";
    cout << "speed up the process. After all current threads completed and before the next\n";
    cout << "set of threads, TreeNET performs refinement on the new subnets to evaluate and\n";
    cout << "\"fix\" them when possible. If one target IP is already encompassed by a\n";
    cout << "previously inferred subnet, it is not investigated further.\n";
    cout << "\n";
    cout << "3) Paris traceroute\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "Paris traceroute is used to obtain a route to each inferred (and refined)\n";
    cout << "subnet. This step also uses multi-threading to speed up the process. At the\n";
    cout << "end of this step, the .subnet output file (listing all inferred subnets, plus\n";
    cout << "responsive IPs, classification and respective route).\n";
    cout << "\n";
    cout << "4) Network tree construction\n";
    cout << "----------------------------\n";
    cout << "\n";
    cout << "The data collected this far is used to build the tree that will locate subnets\n";
    cout << "with respect to each other. This step is entirely passive and is a\n";
    cout << "requirement for alias resolution.\n";
    cout << "\n";
    cout << "5) Alias resolution hints collection\n";
    cout << "------------------------------------\n";
    cout << "\n";
    cout << "The tree is used to isolate candidate IPs for alias resolution, which are\n";
    cout << "probed again to obtain alias resolution \"hints\" - data that will help to\n";
    cout << "fingerprint them and select the best alias resolution method to associate\n";
    cout << "together IPs bordering a same neighborhood.\n";
    cout << "\n";
    cout << "6) Alias resolution and tree interpretation\n";
    cout << "-------------------------------------------\n";
    cout << "\n";
    cout << "The data collected at the previous step is used to carry out the actual alias\n";
    cout << "resolution. At the end of this step, the whole tree is analyzed in-depth and\n"; 
    cout << "TreeNET outputs 3 additionnal files:\n";
    cout << "-a .ip file, providing all responsive IPs (both from pre-scanning and others\n";
    cout << " appearing in routes),\n";
    cout << "-a .fingerprint file, listing all candidate IPs for alias resolution and their\n";
    cout << " respective fingerprint,\n";
    cout << "-an .alias file, which provides all the aliases that were obtained.\n";
    cout << "This step is entirely passive.\n";
    cout << "\n";
    
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "N.B.: as Arborist is built on the original TreeNET, it is simply denoted as\n";
    cout << "\"TreeNET\" in the next lines.\n";
    cout << "\n";
    cout << "You can use TreeNET as follows:\n";
    cout << "\n";
    cout << "./treenet [target n°1],[target n°2],[...]\n";
    cout << "\n";
    cout << "where each target can be:\n";
    cout << "-a single IP,\n";
    cout << "-a whole IP block (in CIDR notation),\n";
    cout << "-a file containing a list of the notations mentioned above, which each item\n";
    cout << " being separated with \\n.\n";
    cout << "\n";
    cout << "You can use various options and flags to handle the settings of TreeNET, such\n";
    cout << "as probing protocol, the amount of threads used in multi-threaded parts of the\n";
    cout << "application, etc. These options and flags are detailed below.\n";
    cout << "\n";
    cout << "Short   Verbose                             Expected value\n";
    cout << "-----   -------                             --------------\n";
    cout << "\n";
    cout << "-e      --probing-egress-interface          IP or DNS\n";
    cout << "\n";
    cout << "Interface name through which probing/response packets exit/enter (default is\n";
    cout << "the first non-loopback IPv4 interface in the active interface list). Use this\n";
    cout << "option if your machine has multiple network interface cards and if you want to\n";
    cout << "prefer one interface over the others.\n";
    cout << "\n";
    cout << "-f      --probing-no-fixed-flow             None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to deactivate the \"Paris traceroute\"\n";
    cout << "technique used while probing. By default, TreeNET always uses it to ensure\n";
    cout << "that remote routers will always use the same path for a given destination,\n";
    cout << "such that the TTL to reach this destination is always the same. Note that the\n";
    cout << "traceroute part of TreeNET (i.e., where the route to the each subnet is\n";
    cout << "measured) always use Paris traceroute, even if this flag is provided.\n";
    cout << "\n";
    cout << "-m      --probing-minimum-hop               Integer from [1,255]\n";
    cout << "\n";
    cout << "Use this option to specify the minimum amount of hops used when evaluating the\n";
    cout << "distance between the vantage point and a target IP in terms of hops (or TTL).\n";
    cout << "By default, it is set to 1.\n";
    cout << "\n";
    cout << "-g      --probing-guess-lan                 None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command-line to explore the LAN (Local Area Network) as\n";
    cout << "a subnet with ExploreNET. By default, the program will use the available\n";
    cout << "information and avoid probing the LAN.\n";
    cout << "\n";
    cout << "-p      --probing-payload-message           String\n";
    cout << "\n";
    cout << "Use this option to edit the message carried in the payload of the probes. This\n";
    cout << "message should just advertise that the probes are not sent for malicious\n";
    cout << "purposes, only for measurements. By default, it states \"NOT AN ATTACK\".\n";
    cout << "\n";
    cout << "-n      --probing-network-address           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to consider network address of a subnet as\n";
    cout << "a non-assignable IP address. By default, it is assumed the network address of\n";
    cout << "a subnet (i.e., first address of the block, with only zero's beyond the subnet\n";
    cout << "mask in binary representation) can be assigned to an interface, because this\n";
    cout << "behaviour has been observed from time to time despite being not recommended.\n";
    cout << "This is mostly relevant during the subnet inference phase.\n";
    cout << "\n";
    cout << "-b      --probing-base-protocol             \"UDP\", \"TCP\" or \"ICMP\"\n";
    cout << "\n";
    cout << "Use this option to specify the base protocol used to probe target addresses.\n";
    cout << "By default, it is ICMP. Note that this is only the \"base\" protocol because\n";
    cout << "some inference techniques (like some alias resolution methods) rely on a\n";
    cout << "precise protocol which is therefore used instead.\n";
    cout << "\n";
    cout << "-r      --probing-regulating-period         Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the minimum amount of milliseconds between two\n";
    cout << "consecutive probes sent by a same thread. By default, this value is set to 50.\n";
    cout << "\n";
    cout << "-s      --probing-second-opinion            None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to always probe a target a second time when\n";
    cout << "the first probe results in a failure. Note that this only applies beyond the\n";
    cout << "network pre-scanning, as the pre-scanning already probes unresponsive targets\n";
    cout << "a second time (and a third time if asked). In other phases, probing a target a\n";
    cout << "second time can mitigate failures over short periods of time, but if the\n";
    cout << "target network is unreliable (i.e., very variable response delays), it can\n";
    cout << "slow down the whole application.\n";
    cout << "\n";
    cout << "-t      --probing-timeout-period            Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the amount of milliseconds spent waiting before\n";
    cout << "diagnosing a timeout. Editing this value will especially impact the network\n";
    cout << "pre-scanning phase (i.e., phase where the liveness of target IPs is checked).\n";
    cout << "By default, this value is set to 2500 (2,5 seconds).\n";
    cout << "\n";
    cout << "-a      --concurrency-amount-threads        Integer (amount of threads)\n";
    cout << "\n";
    cout << "Use this option to edit the amount of threads used in any multi-threaded part\n";
    cout << "of TreeNET (network pre-scanning, network scanning/subnet inference, subnet\n";
    cout << "refinement, Paris traceroute to each subnet and alias resolution hints\n";
    cout << "collection). By default, this value is set to 256.\n";
    cout << "\n";
    cout << "WARNING: due to scheduling strategies used during alias resolution, this value\n";
    cout << "SHOULD NOT be smaller than the amount of collected IP IDs per IP (from [3,20])\n";
    cout << "plus one. Using less threads would lead to malfunction and less efficient\n";
    cout << "alias resolution.\n";
    cout << "\n";
    cout << "-d      --concurrency-delay-threading       Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the minimum amount of milliseconds spent waiting\n";
    cout << "between two immediately consecutive threads. This value is set to 250 (i.e.,\n";
    cout << "0,25s) by default. This delay is meant to avoid sending too many probes at the\n";
    cout << "same time in parts of TreeNET involving multi-threading, such as subnet\n";
    cout << "inference. Indeed, a small delay for a large amount of threads could\n";
    cout << "potentially cause congestion and make the whole application ineffective.\n";
    cout << "\n";
    cout << "-u      --pre-scanning-use-expansion        None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to perform expansion during the\n";
    cout << "pre-scanning phase, that is, any single IP or small IP block (i.e., prefix\n";
    cout << "length is longer than 20) will be replaced by the /20 block encompassing it\n";
    cout << "to ensure neighbor IPs that could be relevant for subnet inference will be\n";
    cout << "considered at pre-scanning.\n";
    cout << "\n";
    cout << "-o      --pre-scanning-third-opinion        None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to trigger a third opinion probing phase\n";
    cout << "during the network pre-scanning. Traditionnaly, TreeNET performs a second\n";
    cout << "opinion with twice the initial timeout to ensure it did not miss responsive\n";
    cout << "IPs because of delays and such. With the third opinion, unresponsive IPs from\n";
    cout << "previous probing are probed again with four times the initial timeout. Users\n";
    cout << "wanting to use this feature should always keep in mind this will inevitably\n";
    cout << "make the pre-scanning phase longer.\n";
    cout << "\n";
    cout << "-w      --alias-resolution-amount-ip-ids    Integer from [3,20]\n";
    cout << "\n";
    cout << "Use this option to edit the amount of IP IDs that are collected for each IP\n";
    cout << "that is candidate for alias resolution. By default, this amount is set to 4.\n";
    cout << "Increasing this value will have an impact on the viability of Ally technique\n";
    cout << "and the accuracy while estimating the velocity range of the IP ID counter\n";
    cout << "associated with a given IP, at the cost of sending more probes. You cannot\n";
    cout << "assign a value smaller than 3 because there needs at least two time intervals\n";
    cout << "between 3 consecutive IP IDs to get a range.\n";
    cout << "\n";
    cout << "WARNING: due to scheduling strategies used during alias resolution, this value\n";
    cout << "SHOULD NOT be greater than the amount of threads being used minus one. Using\n";
    cout << "less threads would lead to malfunction and less efficient alias resolution.\n";
    cout << "\n";
    cout << "-x      --alias-resolution-rollovers-max    Integer from [1,256]\n";
    cout << "\n";
    cout << "Use this option to edit the maximum amount of rollovers considered while\n";
    cout << "estimating the speed at which an IP ID counter evolves. This, of course, is\n";
    cout << "only relevant for the alias resolution technique based on velocity. By\n";
    cout << "default, TreeNET considers a maximum of 10.\n";
    cout << "\n";
    cout << "-y      --alias-resolution-range-tolerance  Positive, non-zero real value\n";
    cout << "\n";
    cout << "Use this option to edit the tolerance value for checking velocity range\n";
    cout << "overlap while using the velocity-based alias resolution technique. Such value\n";
    cout << "is used to extend the largest interval between two intervals being compared\n";
    cout << "so that very close, yet non-overlapping intervals (say, [0.5,0.55] and\n";
    cout << "[0,56, 0.6]) can be associated together. By default, it is 0.2. Increasing\n";
    cout << "this value might, however, increase the amount of false positives while\n";
    cout << "aliasing IPs together.\n";
    cout << "\n";
    cout << "-z      --alias-resolution-error-tolerance  Real from [0,0.5[\n";
    cout << "\n";
    cout << "Use this option to edit the tolerance value for rouding errors while\n";
    cout << "estimating the velocity of an IP ID counter. After computing the potential\n";
    cout << "values for rollovers that happened between each pair of consecutive IP IDs\n";
    cout << "as real values, the error after rounding them to get integer values should\n";
    cout << "stay below a threshold to ensure the soundness of the values. By default, this\n";
    cout << "threshold is set to 0.35.\n";
    cout << "\n";
    cout << "-l      --label-output                      String\n";
    cout << "\n";
    cout << "Use this option to edit the label that will be used to name the various output\n";
    cout << "files produced by TreeNET (usually ended with a suffix related to their\n";
    cout << "content: .subnet for the subnet dump, .ip for the IP dictionnary with alias\n";
    cout << "resolution hints, etc.). By default, TreeNET will use the time at which it\n";
    cout << "completes network scanning (i.e., subnet inference and refinement mixed\n";
    cout << "together) in the format dd-mm-yyyy hh:mm:ss.\n";
    cout << "\n";
    cout << "-j      --join-explorenet-records           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to get an additionnal output file, ending\n";
    cout << "with the extension .xnet, which will list target IPs along the subnets that\n";
    cout << "were obtained from them during the inference (BEFORE refinement). Moreover,\n";
    cout << "the subnet positioning cost (SPC), i.e. amount of probes to position the\n";
    cout << "subnet, the subnet inference cost (SIC), i.e. amount of probes to infer this\n";
    cout << "subnet, and the alternative subnet, if any, are also provided. These pieces of\n";
    cout << "info are normally provided in ExploreNET v2.1 but eventually disappeared from\n";
    cout << "the regular output of TreeNET. This flag allows you to get these details\n";
    cout << "in addition to the typical output files, which can be meaningful in some\n";
    cout << "situations.\n";
    cout << "\n";
    cout << "-v      --verbosity                         0, 1, 2 or 3\n";
    cout << "\n";
    cout << "Use this option to handle the verbosity of the console output produced by\n";
    cout << "TreeNET. Each accepted value corresponds to a \"mode\":\n";
    cout << "\n";
    cout << "* 0: it is the default verbosity. In this mode, TreeNET only displays the most\n";
    cout << "  significant details about progress. For instance, during the Paris\n";
    cout << "  traceroute phase, it only displays the subnets for which the route has been\n";
    cout << "  computed, but not the route it obtained (of course, it is still written in\n";
    cout << "  the .subnet output file).\n";
    cout << "\n";
    cout << "* 1: this is the \"slightly verbose\" mode. TreeNET displays more algorithmic\n";
    cout << "  details and results. For instance, it displays the routes it obtained during\n";
    cout << "  the Paris traceroute step, the different steps of the alias resolution hint\n";
    cout << "  collection process, etc.\n";
    cout << "\n";
    cout << "* 2: this mode stacks up on the previous and adds more details about the\n";
    cout << "  subnet inference as carried out by ExploreNET (before refinement) by dumping\n";
    cout << "  in the console a short log from a thread that inferred a subnet from a given\n";
    cout << "  target IP.\n";
    cout << "\n";
    cout << "* 3: again stacking up on the previous, this last mode also dumps small logs\n";
    cout << "  for each probe once it has been carried out. It is equivalent to a debug\n";
    cout << "  mode.\n";
    cout << "\n";
    cout << "  WARNING: this debug mode is extremely verbose and should only be considered\n";
    cout << "  for unusual situations where investigating the result of each probe becomes\n";
    cout << "  necessary. Do not use it for large-scale work, like the analysis of a whole\n";
    cout << "  AS, as redirecting the console output to files might produce unnecessarily\n";
    cout << "  large files.\n";
    cout << "\n";
    cout << "-c      --credits                           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to display the version of TreeNET and some\n";
    cout << "credits. TreeNET will not run further in this case, though -h and -i flags can\n";
    cout << "be used in addition.\n";
    cout << "\n";
    cout << "-h      --help                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to know how to use TreeNET and see the\n";
    cout << "complete list of options and flags and how they work. TreeNET will not run\n";
    cout << "further after displaying this, though -c and -i flags can be used in addition.\n";
    cout << "\n";
    cout << "-i      --info                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to read a summary of how TreeNET works,\n";
    cout << "with a list of the different algorithm steps. Keep in mind, however, that this\n";
    cout << "is only a summary and that you will learn much more on the way TreeNET works\n";
    cout << "by reading related papers or even the source code. TreeNET will not run\n";
    cout << "further after displaying this, though -c and -h flags can be used in addition.\n";
    cout << "\n";
    
    cout.flush();
}

void printVersion()
{
    cout << "Version and credits\n";
    cout << "===================\n";
    cout << "\n";
    cout << "TreeNET v3.0 \"Arborist\", written by Jean-François Grailet (08 - 09/2016).\n";
    cout << "Based on ExploreNET version 2.1, copyright (c) 2013 Mehmet Engin Tozal.\n";
    cout << "\n";
    
    cout.flush();
}

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default parameters (can be edited by user)
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
    unsigned short maxRollovers = 10; // Idem
    double baseTolerance = 0.2; // Idem
    double maxError = 0.35; // Idem
    bool doubleProbe = false;
    bool useFixedFlowID = true;
    bool thirdOpinionPrescan = false;
    bool prescanExpand = false; 
    bool saveExploreNETRecords = false;
    unsigned short displayMode = TreeNETEnvironment::DISPLAY_MODE_LACONIC;
    unsigned short nbThreads = 256;
    string outputFileName = ""; // Gets a default value later if not set by user.
    
    // Values to check if info, usage, version... should be displayed.
    bool displayInfo = false, displayUsage = false, displayVersion = false;
    
    /*
     * PARSING ARGUMENT
     * 
     * The main argument (target prefixes or input files, each time separated by commas) can be 
     * located anywhere. To make things simple for getopt_long(), argv is processed to find it and 
     * put it at the end. If not found, the program stops and displays an error message.
     */
    
    int totalArgs = argc;
    string targetsStr = ""; // List of targets (input files or plain targets)
    bool found = false;
    bool flagParam = false; // True if a value for a parameter is expected
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'c':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'n':
                case 'o':
                case 's':
                case 'u':
                    break;
                default:
                    flagParam = true;
                    break;
            }
        }
        else if(flagParam)
        {
            flagParam = false;
        }
        else
        {
            // Argument found!
            char *arg = argv[i];
            for(int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argv[argc - 1] = arg;
            found = true;
            totalArgs--;
            break;
        }
    }
    
    targetsStr = argv[argc - 1];   
    
    /*
     * PARSING PARAMETERS
     *
     * In addition to the main argument parsed above, TreeNET provides various input flags which 
     * can be used to handle the probing parameters, concurrency parameters, alias resolution 
     * parameters and a few more features (like setting the message sent with the ICMP probes and 
     * the label of the output files).
     */
     
    int opt = 0;
    int longIndex = 0;
    const char* const shortOpts = "a:b:cd:e:fghijl:m:nop:r:st:uv:w:x:y:z:";
    const struct option longOpts[] = {
            {"probing-egress-interface", required_argument, NULL, 'e'}, 
            {"probing-no-fixed-flow", no_argument, NULL, 'f'}, 
            {"probing-minimum-hop", required_argument, NULL, 'm'}, 
            {"probing-guess-lan", no_argument, NULL, 'g'}, 
            {"probing-payload-message", required_argument, NULL, 'p'}, 
            {"probing-network-address", no_argument, NULL, 'n'}, 
            {"probing-base-protocol", required_argument, NULL, 'b'}, 
            {"probing-regulating-period", required_argument, NULL, 'r'}, 
            {"probing-second-opinion", no_argument, NULL, 's'}, 
            {"probing-timeout-period", required_argument, NULL, 't'}, 
            {"concurrency-amount-threads", required_argument, NULL, 'a'}, 
            {"concurrency-delay-threading", required_argument, NULL, 'd'}, 
            {"pre-scanning-use-expansion", no_argument, NULL, 'u'}, 
            {"pre-scanning-third-opinion", no_argument, NULL, 'o'}, 
            {"alias-resolution-amount-ip-ids", required_argument, NULL, 'w'}, 
            {"alias-resolution-rollovers-max", required_argument, NULL, 'x'}, 
            {"alias-resolution-range-tolerance", required_argument, NULL, 'y'}, 
            {"alias-resolution-error-tolerance", required_argument, NULL, 'z'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"join-explorenet-records", no_argument, NULL, 'j'}, 
            {"verbosity", required_argument, NULL, 'v'}, 
            {"credits", no_argument, NULL, 'c'}, 
            {"help", no_argument, NULL, 'h'}, 
            {"info", no_argument, NULL, 'i'}
    };
    
    string optargSTR;
    unsigned long val;
    unsigned long sec;
    unsigned long microSec;
    double valDouble;
    while((opt = getopt_long(totalArgs, argv, shortOpts, longOpts, &longIndex)) != -1)
    {
        /*
         * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
         * Otherwise, it prevents the code from recognizing flags like -v, -h or -g (because they 
         * require no argument) and make it throw an exception... To avoid this, a second switch 
         * is used.
         *
         * (this is noteworthy, as this error is still present in ExploreNET v2.1)
         */
        
        switch(opt)
        {
            case 'c':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'n':
            case 'o':
            case 's':
            case 'u':
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
                    cout << "Error for -e option: cannot obtain any IP address ";
                    cout << "assigned to the interface \"" + optargSTR + "\". ";
                    cout << "Please fix the argument for this option before ";
                    cout << "restarting TreeNET.\n" << endl;
                    return 1;
                }
                break;
            case 'f':
                useFixedFlowID = false;
                break;
            case 'm':
                inputStartTTL = StringUtils::string2Uchar(optargSTR);
                break;
            case 'g':
                exploreLANExplicitly = true;
                break;
            case 'p':
                probeAttentionMessage = optargSTR;
                break;
            case 'n':
                useLowerBorderAsWell = false;
                break;
            case 'b':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("UDP"))
                    probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_UDP;
                else if(optargSTR == string("TCP"))
                    probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_TCP;
                else if(optargSTR != string("ICMP"))
                {
                    cout << "Warning for option -b: unrecognized protocol " << optargSTR;
                    cout << ". Please select a protocol between the following three: ";
                    cout << "ICMP, UDP and TCP. Note that ICMP is the default base ";
                    cout << "protocol.\n" << endl;
                }
                break;
            case 'r':
                val = 1000 * StringUtils::string2Ulong(optargSTR);
                if(val > 0)
                {
                    sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                    microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                    probeRegulatingPeriod.setTime(sec, microSec);
                }
                else
                {
                    cout << "Warning for -r option: a negative value (or 0) was parsed. ";
                    cout << "TreeNET will use the default value for the probe regulating ";
                    cout << "period (= 50ms).\n" << endl;
                }
                break;
            case 's':
                doubleProbe = true;
                break;
            case 't':
                val = 1000 * StringUtils::string2Ulong(optargSTR);
                if(val > 0)
                {
                    sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                    microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                    timeoutPeriod.setTime(sec, microSec);
                }
                else
                {
                    cout << "Warning for -r option: a negative value (or 0) was parsed. ";
                    cout << "TreeNET will use the default value for the timeout period ";
                    cout << "(= 2,5s).\n" << endl;
                }
                break;
            case 'a':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb < 32767)
                {
                    nbThreads = (unsigned short) gotNb;
                }
                else
                {
                    cout << "Warning for -a option: a value smaller than 1 or greater ";
                    cout << "than 32766 was parsed. TreeNET will use the default amount ";
                    cout << "of threads (= 256).\n" << endl;
                    break;
                }
                
                if (gotNb < (nbIPIDs + 1))
                {
                    nbThreads = 256;
                    
                    cout << "Warning for -a option: a value that is positive but smaller than ";
                    cout << "the amount of IP IDs being collected per IP plus one was parsed. ";
                    cout << "Such a configuration is forbidden because of the scheduling ";
                    cout << "strategies used in the alias resolution step for IP ID collection. ";
                    cout << "TreeNET will use the default amount of threads (= 256).\n" << endl;
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
                else
                {
                    cout << "Warning for -d option: a negative value (or 0) was parsed. ";
                    cout << "TreeNET will use the default value for the delay between the ";
                    cout << "launch of two consecutive threads (= 250ms).\n" << endl;
                }
                break;
            case 'u':
                prescanExpand = true;
                break;
            case 'o':
                thirdOpinionPrescan = true;
                break;
            case 'w':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 2 && gotNb <= 20)
                {
                    nbIPIDs = (unsigned short) gotNb;
                }
                else
                {
                    cout << "Warning for -w option: a value outside the suggested range (i.e., ";
                    cout << "[3,20]) was provided. TreeNET will use the default value for this ";
                    cout << "option (= 4).\n" << endl;
                    break;
                }
                
                if ((gotNb + 1) > nbThreads)
                {
                    nbIPIDs = nbThreads - 1;
                    
                    cout << "Warning for -w option: a value that is positive but greater than ";
                    cout << "the amount of concurrent threads was parsed. Such a configuration ";
                    cout << "is forbidden because of the scheduling strategies used in the alias ";
                    cout << "resolution step for IP ID collection. TreeNET will use the current ";
                    cout << "amount of threads minus one for the amount of collected IP IDs per ";
                    cout << "IP.\n" << endl;
                }
                
                break;
            case 'x':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb <= 256)
                {
                    maxRollovers = (unsigned short) gotNb;
                }
                else
                {
                    cout << "Warning for -x option: a value outside the suggested range (i.e., ";
                    cout << "[1,256]) was provided. TreeNET will use the default value for this ";
                    cout << "option (= 10).\n" << endl;
                }
                break;
            case 'y':
                valDouble = StringUtils::string2double(optargSTR);
                if(valDouble >= 0.0)
                {
                    baseTolerance = valDouble;
                }
                else
                {
                    cout << "Warning for -y option: a negative value was provided. TreeNET will ";
                    cout << "use the default value for this option (= 0.2).\n" << endl;
                }
                break;
            case 'z':
                valDouble = StringUtils::string2double(optargSTR);
                if(valDouble >= 0.0 && valDouble < 0.5)
                {
                    maxError = valDouble;
                }
                else
                {
                    cout << "Warning for -x option: a value outside the suggested range (i.e., ";
                    cout << "[0,0.5[) was provided. TreeNET will use the default value for this ";
                    cout << "option (= 0.35).\n" << endl;
                }
                break;
            case 'l':
                outputFileName = optargSTR;
                break;
            case 'j':
                saveExploreNETRecords = true;
            case 'v':
                gotNb = std::atoi(optargSTR.c_str());
                if(gotNb >= 0 && gotNb <= 3)
                    displayMode = (unsigned short) gotNb;
                else
                {
                    cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                    cout << "out of [0,3]) was provided. TreeNET will use the laconic ";
                    cout << "mode (default mode).\n" << endl;
                }
                break;
            case 'c':
                displayVersion = true;
                break;
            case 'h':
                displayUsage = true;
                break;
            case 'i':
                displayInfo = true;
                break;
            default:
                break;
        }
    }
    
    if(displayInfo || displayUsage || displayVersion)
    {
        if(displayInfo)
            printInfo();
        if(displayUsage)
            printUsage();
        if(displayVersion)
            printVersion();
        return 0;
    }
    
    if(!found)
    {
        cout << "No target prefix or target file was provided. Use -h or --help to get more ";
        cout << "details on how to use TreeNET." << endl;
        return 0;
    }
    
    /*
     * SETTING THE ENVIRONMENT
     *
     * Before listing target IPs, the initialization of TreeNET is completed by getting the local 
     * IP and the local subnet mask and creating a TreeNETEnvironment object, a singleton which 
     * will be passed by pointer to other classes of the program to be able to get all the 
     * current settings, which are either default values either values parsed in the parameters 
     * provided by the user. The singleton also provides access to data structures other classes 
     * should be able to access.
     */

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
                                                     inputStartTTL, 
                                                     probingProtocol, 
                                                     exploreLANExplicitly, 
                                                     useLowerBorderAsWell, 
                                                     doubleProbe, 
                                                     useFixedFlowID, 
                                                     prescanExpand, 
                                                     saveExploreNETRecords, 
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
                                                     displayMode, 
                                                     nbThreads);
    
    // Gets direct access to the subnet set
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    SubnetSiteSet *zonesToAvoid = env->getIPBlocksToAvoid();

    // Various variables/structures which should be considered when catching some exception
    unsigned short sizeArray = 0;
    Thread **th = NULL;
    SubnetRefiner *sr = NULL;
    
    try
    {
        // Parses inputs and gets target lists
        TargetParser *parser = new TargetParser(env);
        
        parser->parseCommandLine(targetsStr);
        
        cout << "TreeNET v3.0 \"Arborist\"\n" << endl;
        
        // Announces that it will ignore LAN.
        if(parser->targetsEncompassLAN())
        {
            cout << "Target IPs encompass the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored.\n" << endl;
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
            cout << "All probed IPs were responsive.\n" << endl;
        }
        
        cout << "Prescanning ended.\n" << endl;

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
            cout << "\n" << endl;
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
        
        cout << "Starting network scanning..." << endl;
        if(!(displayMode == TreeNETEnvironment::DISPLAY_MODE_DEBUG || displayMode == TreeNETEnvironment::DISPLAY_MODE_VERBOSE))
            cout << endl;
        
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
                cout << "New subnets found by the previous " << sizeArray << " threads:\n";
                cout << newSubnets << endl;
            }
            else
            {
                cout << "Previous " << sizeArray << " threads found no new subnet\n" << endl;
            }
            
            // Performs refinement
            if(needsRefinement)
            {
                cout << "Refining incomplete subnets...\n" << endl;
                
                SubnetSite *candidateForRefinement = subnetSet->getIncompleteSubnet();
                while(candidateForRefinement != NULL)
                {
                    /*
                     * Checks if expansion should be conducted, i.e. the subnet should not be 
                     * encompassed by an UNDEFINED subnet found in the "IPBlocksToAvoid" set from 
                     * TreeNETEnvironment. Otherwise, expansion is a waste of time: the presence 
                     * of an UNDEFINED subnet means we already looked for Contra-Pivot interfaces 
                     * in this zone without success. There should not be any issue regarding TTL 
                     * values, because expansion should have stopped before reaching the UNDEFINED 
                     * state in this case. When refinement by expansion is not done, the subnet is 
                     * directly labelled as SHADOW and reinserted in subnetSet.
                     */
                    
                    SubnetSite *ss2 = zonesToAvoid->isSubnetEncompassed(candidateForRefinement);
                    if(ss2 != NULL)
                    {
                        string ssStr = candidateForRefinement->getInferredNetworkAddressString();
                        string ss2Str = ss2->getInferredNetworkAddressString();
                        cout << "No refinement for " << ssStr << ": it is encompassed in the ";
                        cout << ss2Str << " IPv4 address block.\nThis block features Pivot ";
                        cout << "interfaces with the same TTL as expected for " << ssStr << ".\n";
                        cout << "It has already been checked to find Contra-Pivot interfaces, ";
                        cout << "without success.\n";
                        cout << ssStr << " marked as SHADOW subnet.\n" << endl;
                    
                        candidateForRefinement->setRefinementStatus(SubnetSite::SHADOW_SUBNET);
                        
                        // No check of return value because subnet did not change
                        subnetSet->addSite(candidateForRefinement); 
                        
                        candidateForRefinement = subnetSet->getIncompleteSubnet();
                        continue;
                    }
                
                    sr->expand(candidateForRefinement);
                    unsigned short res = subnetSet->addSite(candidateForRefinement);
                    
                    if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
                    {
                        delete candidateForRefinement;
                        candidateForRefinement = NULL;
                    }
                
                    candidateForRefinement = subnetSet->getIncompleteSubnet();
                }
                
                /*
                 * If we are in laconic display mode, we add a line break before the next message 
                 * to keep the display pretty to read.
                 */
                
                if(displayMode == TreeNETEnvironment::DISPLAY_MODE_LACONIC)
                    cout << "\n";
                
                cout << "Back to scanning...\n" << endl;
            }
            
            // Updates number of targets for next salve of threads
            if(nbTargets > (unsigned long) sizeArray)
            {
                nbTargets -= (unsigned long) sizeArray;
                if(nbTargets < (unsigned long) sizeArray)
                    sizeArray = (unsigned short) nbTargets;
            }
            else
                nbTargets = 0;
        }
        
        delete[] th;
        
        // Restores regular timeout
        if(editedTimeout)
        {
            env->setTimeoutPeriod(timeoutPeriod);
        }
        
        cout << "Scanning completed.\n" << endl;
        
        /*
         * A first output file can be written at this point. The names used for all output files 
         * are already defined in the following lines.
         */
        
        string newFileNameXnet = "";
        string newFileNameSubnet = "";
        string newFileNameIP = "";
        string newFileNameAlias = "";
        string newFileNameFingerprints = "";
        if(outputFileName.length() > 0)
        {
            newFileNameXnet = outputFileName + ".xnet";
            newFileNameSubnet = outputFileName + ".subnet";
            newFileNameIP = outputFileName + ".ip";
            newFileNameAlias = outputFileName + ".alias";
            newFileNameFingerprints = outputFileName + ".fingerprint";
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
            
            newFileNameXnet = timeStr + ".xnet";
            newFileNameSubnet = timeStr + ".subnet";
            newFileNameIP = timeStr + ".ip";
            newFileNameAlias = timeStr + ".alias";
            newFileNameFingerprints = timeStr + ".fingerprint";
        }
        
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
            cout << "Refinement by filling (passive method)...\n" << endl;
            
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
            
            /*
             * Like at the end of Bypass round, we have to take into account the display mode 
             * before adding a new line break to space harmoniously the different steps.
             */
            
            if(displayMode != TreeNETEnvironment::DISPLAY_MODE_LACONIC)
                cout << endl;
        }
        
        // Shadow expansion (no parallelization here, because this operation is instantaneous)
        if(nbShadows > 0)
        {
            cout << "Expanding shadow subnets to the maximum...\n" << endl;
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
                unsigned short res = subnetSet->addSite((*it));
                if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
                {
                    delete (*it);
                }
            }
        }
        
        delete sr; // SubnetRefiner no longer needed
        
        // Saves subnet inference details if user asked them.
        if(saveExploreNETRecords)
        {
            env->outputExploreNETRecords(newFileNameXnet);
            cout << "Details about subnet inference as carried out by ExploreNET have been ";
            cout << "written in a new file " << newFileNameXnet << ".\n" << endl;
        }
        
        // 1st save of the inferred subnets (no route)
        subnetSet->outputAsFile(newFileNameSubnet);
        cout << "Inferred subnets have been saved in an output file ";
        cout << newFileNameSubnet << ".\n" << endl;
        
        // Outputting results with the set (simple display).
        cout << left << setw(25) << "Subnet";
        cout << left << setw(15) << "Class" << endl;
        cout << left << setw(25) << "------";
        cout << left << setw(15) << "-----" << endl;
        
        for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
        {
            cout << left << setw(25) << (*it)->getInferredNetworkAddressString();
            cout << left << setw(15);
            switch((*it)->getRefinementStatus())
            {
                case SubnetSite::ACCURATE_SUBNET:
                    cout << "Accurate";
                    break;
                case SubnetSite::ODD_SUBNET:
                    cout << "Odd";
                    break;
                case SubnetSite::SHADOW_SUBNET:
                    cout << "Shadow";
                    break;
                default:
                    cout << "Undefined";
                    break;
            }
            cout << endl;
        }
        
        /*
         * STEP V: SUBNET NEIGHBORHOOD INFERENCE
         *
         * Before inferring L2/L3 devices, we locate subnets regarding each other with a tree. 
         * The process of constructing the tree, as of September 2016, occurs in two steps: first 
         * step is the preparation, which is a preliminary probing task to collect additionnal 
         * details on the subnets and their location (for example, running Paris traceroute). 
         * Then the construction (or tree growth) actually starts, using these preliminary pieces 
         * of info. The construction itself can be enterily passive (i.e., no probing) or involve 
         * additionnal probing work.
         */
         
        cout << "\nPreparing the tree growth...\n" << endl;
        
        Grower *g = new ClassicGrower(env);
        g->prepare();
        
        cout << "Preparation complete.\n\nGrowing network tree..." << endl;
        
        g->grow();
        
        // End of tree growth
        cout << "Growth complete.\n" << endl;
        
        Soil *result = g->getResult();
        delete g;
        
        // Final save of the subnets, overriding previous save.
        result->outputSubnets(newFileNameSubnet);
        cout << "Inferred subnets have been saved a second time (with routing details) in ";
        cout << newFileNameSubnet << ".\n" << endl;
        
        if(displayMode != TreeNETEnvironment::DISPLAY_MODE_LACONIC)
        {
            Climber *robin = new Robin(env);
            robin->climb(result);
            delete robin;
        }
        
        /*
         * STEP VI: ALIAS RESOLUTION
         *
         * Interfaces bordering a neighborhood are probed once more to attempt to gather 
         * interfaces as routers.
         */
        
        cout << "Alias resolution...\n" << endl;
        
        // Collects alias resolution hints.
        Climber *cuckoo = new Cuckoo(env);
        cuckoo->climb(result);
        cout << endl;
        delete cuckoo;
        
        // Internal node exploration and actual alias resolution are only done now.
        Crow *c = new Crow(env);
        c->climb(result);
        c->outputAliases(newFileNameAlias);
        delete c;
        
        cout << "Inferred alias lists have been saved in an output file ";
        cout << newFileNameAlias << ".\n";
        
        env->getIPTable()->outputDictionnary(newFileNameIP);
        cout << "IP dictionnary with alias resolution hints has been saved in an output file ";
        cout << newFileNameIP << ".\n";
        
        env->getIPTable()->outputFingerprints(newFileNameFingerprints);
        cout << "IP dictionnary with fingerprints has been saved in an output file ";
        cout << newFileNameFingerprints << "." << endl;

        delete result;
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
        
        delete env;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "Use \"--help\" or \"-h\" parameter to reach help" << endl;
        delete env;
        return 1;
    }
    
    delete env;
    return 0;
}
