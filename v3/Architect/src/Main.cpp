#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::flush;
using std::exit;
#include <iomanip> // For the display of inferred/refined subnets
using std::left;
using std::setw;
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <list>
using std::list;
using std::iterator;
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
#include "treenet/utils/SubnetParser.h"
#include "treenet/utils/IPDictionnaryParser.h"
#include "treenet/tree/NetworkTree.h"
#include "treenet/tree/growth/classic/ClassicGrower.h"
#include "treenet/tree/climbers/Robin.h"
#include "treenet/tree/climbers/Crow.h"
#include "treenet/tree/climbers/Cat.h"
#include "treenet/tree/climbers/Ant.h"
#include "treenet/tree/climbers/Termite.h"
#include "treenet/graph/type-ns/NSProcesser.h"
#include "treenet/graph/type-ers/ERSProcesser.h"

// Simple function to display usage.

void printInfo()
{
    cout << "Summary\n";
    cout << "=======\n";
    cout << "\n";
    cout << "TreeNET \"Architect\" is a companion software for TreeNET \"Arborist\". While the\n";
    cout << "Arborist version is designed exclusively for probing and active processing of\n";
    cout << "the collected data, the Architect version is designed to parse output files\n";
    cout << "from Arborist and rebuild the network tree to eventually transform it into\n";
    cout << "other formats which are better suited for analysis. In particular, Architect\n";
    cout << "converts the data into bipartite formats. It also provides some basic analysis\n";
    cout << "tools, and is overall entirely passive (i.e., there is no probing at all with\n";
    cout << "Architect).\n";
    cout << "\n";
    cout << "Like its probing brother, TreeNET \"Architect\" consists in a sequence of precise\n";
    cout << "algorithmic steps which are described below.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "0) Launch and parsing\n";
    cout << "---------------------\n";
    cout << "\n";
    cout << "Architect parses options and flags (if any) and its main argument which should\n";
    cout << "provide a single dataset the user wants to work with. The dataset should be\n";
    cout << "provided as the name prefix of multiple files, as Architect will automatically\n";
    cout << "append this prefix with the right extensions (.subnet and .ip) to get all\n";
    cout << "relevant parts of a given dataset for the subsequent steps. This way of\n";
    cout << "parsing input is identical to Forester, except that there should not be\n";
    cout << "multiple input datasets.\n";
    cout << "\n";
    cout << "1) Network tree construction\n";
    cout << "----------------------------\n";
    cout << "\n";
    cout << "Architect builds the network tree solely from the data it got from parsing.\n";
    cout << "This step is a pre-requisite for alias resolution.\n";
    cout << "\n";
    cout << "2) Alias resolution\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "The data provided is used to carry out the actual alias resolution, maybe with\n";
    cout << "edited parameters (-x, -y and -z options, cf. usage).\n";
    cout << "\n";
    cout << "3) Network tree display and analysis (partially optional)\n";
    cout << "---------------------------------------------------------\n";
    cout << "\n";
    cout << "Architect displays an in-depth analysis of the tree (i.e., it displays\n";
    cout << "detailed data for each neighborhood) in the console. If the display mode is\n";
    cout << "\"slightly verbose\" or above, it will also display the structure of the tree in\n";
    cout << "console prior to that analysis.\n";
    cout << "\n";
    cout << "4) Bipartite conversion (optional)\n";
    cout << "----------------------------------\n";
    cout << "\n";
    cout << "If requested by the user, Architect processes the network tree to transform it\n";
    cout << "into a (double) bipartite graph. This graph is provided in an output file\n";
    cout << "which first lists each component, denoted by a unique identifier (for example,\n";
    cout << "a subnet is denoted by the notation \"SX\" where X is a natural number), one per\n";
    cout << "line. It then lists the links that exist between such components with the\n";
    cout << "\"[component left] - [component right]\" notation (one link per line).\n";
    cout << "\n";
    cout << "5) Statistics computation (optional)\n";
    cout << "------------------------------------\n";
    cout << "\n";
    cout << "If asked by the user at start, Architect ends its execution by displaying\n";
    cout << "various statistics about the network tree it previously built and the subnets\n";
    cout << "it contains.";
    cout << "\n";
    
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "You can use Architect as follows:\n";
    cout << "\n";
    cout << "./treenet_architect [input file prefix]\n";
    cout << "\n";
    cout << "Here, the \"input file prefix\" denotes the name of any file from a dataset\n";
    cout << "obtained via Arborist without any suffix (.subnet, .ip, .alias, etc.). Indeed,\n";
    cout << "Architect will automatically append the right suffix for each file it should\n";
    cout << "parse.\n";
    cout << "\n";
    cout << "You can use various options and flags to handle the settings of Architect,\n";
    cout << "such as the alias resolution parameters, or to request additionnal services,\n";
    cout << "like a bipartite converson. These options and flags are detailed below.\n";
    cout << "\n";
    cout << "Short   Verbose                             Expected value\n";
    cout << "-----   -------                             --------------\n";
    cout << "\n";
    cout << "-m      --parsing-check-merging              None (flag)\n";
    cout << "\n";
    cout << "Because datasets can get quite large, checking at each newly parsed subnet or\n";
    cout << "IP that there is a collision between the new item and any item previously\n";
    cout << "parsed (for instance, a new subnet overlaps a formerly parsed subnet), in\n"; 
    cout << "which case both items should be merged, can be a waste of time. By default,\n";
    cout << "Architect will not check for such collisions as it aims at parsing a single\n";
    cout << "dataset at a time as produced by other versions of TreeNET, which are supposed\n";
    cout << "to be free of any redudancy. But if collision checking is needed for some\n";
    cout << "reason, you can add this flag to your command line.\n";
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
    cout << "-x      --alias-resolution-rollovers-max    Integer from [1,256]\n";
    cout << "\n";
    cout << "Use this option to edit the maximum amount of rollovers considered while\n";
    cout << "estimating the speed at which an IP ID counter evolves. This, of course, is\n";
    cout << "only relevant for the alias resolution technique based on velocity. By\n";
    cout << "default, Architect considers a maximum of 10.\n";
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
    cout << "Use this option to edit the label that will be used to name the possible\n";
    cout << "output files (containing either a bipartite representation, either statistics\n";
    cout << "on the network tree) produced by Architect. By default, Architect will use the\n";
    cout << "time at which it needs to output a file (for instance, after alias resolution\n";
    cout << "hint collection) in the format dd-mm-yyyy hh:mm:ss.\n";
    cout << "\n";
    cout << "-s      --statistics                        None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to request Architect to compute statistics\n";
    cout << "on the network tree and the subnets it contain. Not only Architect will\n";
    cout << "display these statistics in console, but it will also write them in an output\n";
    cout << "file titled \"Statistics [label]\" (see -l flag to know how you can modify\n";
    cout << "[label] and what is its default value).\n";
    cout << "\n";
    cout << "-t      --output-tree                       None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to request the text equivalent of the\n";
    cout << "network tree after the end of the neighborhood inference step (or tree\n";
    cout << "construction). This will produce a new file \"[label].tree\" providing the\n";
    cout << "requested output.\n";
    cout << "\n";
    cout << "-a      --output-tree-analysis              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to request in-depth tree (neighborhoods)\n";
    cout << "analysis after the end of the alias resolution step. This will produce a new\n";
    cout << "file \"[label].neighborhoods\" providing the requested analysis.\n";
    cout << "\n";
    cout << "-o      --output-datalink-estimation        None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to request L2 estimation after the end of\n";
    cout << "the alias resolution step. This will produce a new file \"[label].l2\" providing\n";
    cout << "the requested analysis.\n";
    cout << "\n";
    cout << "-e      --bipartite-ers                     None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to request conversion of the network tree\n";
    cout << "into an Ethernet switch - router - subnet double bipartite graph (or ERS\n";
    cout << "graph). Note that upon requesting a router graph, an ERS graph will\n";
    cout << "necessarily be generated first (because it's a projection). The output file\n";
    cout << "will be named \"[label].ers-graph\" (see -l flag for [label]).\n";
    cout << "\n";
    cout << "-n      --bipartite-ns                      None (flag)\n";
    cout << "\n";
    cout << "Add this option to your command line to request conversion of the network tree\n";
    cout << "into a neighborhood - subnet bipartite graph (or NS graph). Note that upon\n";
    cout << "requesting a subnet or neighborhood graph, a NS graph will necessarily be\n";
    cout << "generated first (because such graphs are projection). The output file will be\n";
    cout << "named \"[label].ns-graph\" (see -l flag for [label]).\n";
    cout << "\n";
    cout << "-g      --graph-neighborhood                None (flag)\n";
    cout << "\n";
    cout << "Add this option to your command line to request conversion of the network tree\n";
    cout << "into a neighborhood graph. The final graph will be a projection on the\n";
    cout << "neighborhoods of a NS graph. The output file will be named \"[label].n-graph\"\n";
    cout << "(see -l flag for [label]).\n";
    cout << "\n";
    cout << "-r      --graph-router                      None (flag)\n";
    cout << "\n";
    cout << "Add this option to your command line to request conversion of the network tree\n";
    cout << "into a router graph. The final graph will be a projection on the routers of an\n";
    cout << "ERS graph. The output file will be named \"[label].r-graph\" (see -l flag for\n";
    cout << "[label]).\n";
    cout << "\n";
    cout << "-u      --graph-subnet                      1 or 2\n";
    cout << "\n";
    cout << "Add this option to your command line to request conversion of the network tree\n";
    cout << "into a subnet graph. The final graph will be a projection on the subnets of an\n";
    cout << "ERS or an NS graph. The argument should be 1 to project from an ERS graph, and\n";
    cout << "2 if one wants to project from a NS graph. The difference between both output\n";
    cout << "is that the first projection should be more accurate when it comes to\n";
    cout << "multi-label nodes, as the router inference taken into account by the ERS graph\n";
    cout << "should disambiguate the possible multiple neighborhoods merged into one. The\n";
    cout << "output file will be named \"[label].s-graph\" (see -l flag for [label]).\n";
    cout << "\n";
    cout << "-v      --verbosity                         0 or 1\n";
    cout << "\n";
    cout << "Use this option to handle the verbosity of the console output produced by\n";
    cout << "Architect. Each accepted value corresponds to a \"mode\":\n";
    cout << "\n";
    cout << "* 0: it is the default verbosity. In this mode, Architect only displays the\n";
    cout << "  typical steps and how much time they take respecitvely.\n";
    cout << "\n";
    cout << "* 1: this is the \"verbose\" mode. In addition to the default output,\n";
    cout << "  Architect will also display detailed errors when parsing is unsuccessful.\n";
    cout << "\n";
    cout << "-c      --credits                           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to display the version of Architect and\n";
    cout << "some credits. Architect will not run further in this case, though -h and -i\n";
    cout << "flags can be used in addition.\n";
    cout << "\n";
    cout << "-h      --help                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to know how to use Architect and see the\n";
    cout << "complete list of options and flags and how they work. Architect will not run\n";
    cout << "further after displaying this, though -c and -i flags can be used in addition.\n";
    cout << "\n";
    cout << "-i      --info                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to read a summary of how Architect works,\n";
    cout << "with a list of the different algorithm steps. Architect will not run\n";
    cout << "further after displaying this, though -c and -h flags can be used in addition.\n";
    cout << "\n";
    
    cout.flush();
}

void printVersion()
{
    cout << "Version and credits\n";
    cout << "===================\n";
    cout << "\n";
    cout << "TreeNET v3.2 \"Architect\", written by Jean-François Grailet (03/2017).\n";
    cout << "Based on ExploreNET version 2.1, copyright (c) 2013 Mehmet Engin Tozal.\n";
    cout << "\n";
    
    cout.flush();
}

// Simple function to get the current time as a string object.

string getCurrentTimeStr()
{
    time_t rawTime;
    struct tm *timeInfo;
    char buffer[80];

    time(&rawTime);
    timeInfo = localtime(&rawTime);

    strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
    string timeStr(buffer);
    
    return timeStr;
}

// Simple function to convert an elapsed time (in seconds) into days/hours/mins/secs format

string elapsedTimeStr(unsigned long elapsedSeconds)
{
    if(elapsedSeconds == 0)
    {
        return "less than one second";
    }

    unsigned short secs = (unsigned short) elapsedSeconds % 60;
    unsigned short mins = (unsigned short) (elapsedSeconds / 60) % 60;
    unsigned short hours = (unsigned short) (elapsedSeconds / 3600) % 24;
    unsigned short days = (unsigned short) elapsedSeconds / 86400;
    
    stringstream ss;
    if(days > 0)
    {
        if(days > 1)
            ss << days << " days ";
        else
            ss << "1 day ";
    }
    if(hours > 0)
    {
        if(hours > 1)
            ss << hours << " hours ";
        else
            ss << "1 hour ";
    }
    if(mins > 0)
    {
        if(mins > 1)
            ss << mins << " minutes ";
        else
            ss << "1 minute ";
    }
    if(secs > 1)
        ss << secs << " seconds";
    else
        ss << secs << " second";
    
    return ss.str();    
}

// Constants to deal with the methods for generating a subnet graph

const unsigned short SUBNET_GRAPH_NONE = 0;
const unsigned short SUBNET_GRAPH_FROM_NS = 1;
const unsigned short SUBNET_GRAPH_FROM_ERS = 2;

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default parameters (can be edited by user)
    unsigned short nbIPIDs = 4; // For Alias Resolution
    unsigned short maxRollovers = 10; // Idem
    double baseTolerance = 0.2; // Idem
    double maxError = 0.35; // Idem
    bool parsingOmitMerging = true;
    unsigned short displayMode = TreeNETEnvironment::DISPLAY_MODE_LACONIC;
    string labelOutputFiles = ""; // Gets a default value later if not set by user.
    bool computeStatistics = false;
    
    // Boolean variables to check which analysis should be outputted.
    bool outputTree = false, outputNeighborhoods = false, outputL2 = false;
    
    // Boolean variables to check which graphs should be generated.
    bool createBipERS = false, createBipNS = false, createNGraph = false, createRGraph = false;
    unsigned short createSGraph = SUBNET_GRAPH_NONE;
    
    // Boolean variables to check if info, usage, version... should be displayed.
    bool displayInfo = false, displayUsage = false, displayVersion = false;
    
    /*
     * PARSING ARGUMENT
     * 
     * The main argument (input files, separated by commas) can be located anywhere. To make 
     * things simple for getopt_long(), argv is processed to find it and put it at the end. If not 
     * found, the program stops and displays an error message.
     */
    
    int totalArgs = argc;
    string inputsStr = ""; // List of input files
    bool found = false;
    bool flagParam = false; // True if a value for a parameter is expected
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'a':
                case 'c':
                case 'e':
                case 'g':
                case 'h':
                case 'i':
                case 'n':
                case 'm':
                case 'o':
                case 'r':
                case 's':
                case 't':
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
    
    inputsStr = argv[argc - 1];   
    
    /*
     * PARSING PARAMETERS
     *
     * In addition to the main argument parsed above, Architect provides various input flags which 
     * can be used to handle the probing parameters, concurrency parameters, alias resolution 
     * parameters and a few more features (like setting the message sent with the ICMP probes and 
     * the label of the output files).
     */
     
    int opt = 0;
    int longIndex = 0;
    const char* const shortOpts = "aceghil:nmorstu:v:w:x:y:z:";
    const struct option longOpts[] = {
            {"parsing-check-merging", no_argument, NULL, 'm'}, 
            {"alias-resolution-amount-ip-ids", required_argument, NULL, 'w'}, 
            {"alias-resolution-rollovers-max", required_argument, NULL, 'x'}, 
            {"alias-resolution-range-tolerance", required_argument, NULL, 'y'}, 
            {"alias-resolution-error-tolerance", required_argument, NULL, 'z'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"statistics", no_argument, NULL, 's'}, 
            {"output-tree", no_argument, NULL, 't'}, 
            {"output-tree-analysis", no_argument, NULL, 'a'}, 
            {"output-datalink-estimation", no_argument, NULL, 'o'}, 
            {"bipartite-ers", no_argument, NULL, 'e'}, 
            {"bipartite-ns", no_argument, NULL, 'n'}, 
            {"graph-neighborhood", no_argument, NULL, 'g'}, 
            {"graph-router", no_argument, NULL, 'r'}, 
            {"graph-subnet", required_argument, NULL, 'u'}, 
            {"verbosity", required_argument, NULL, 'v'}, 
            {"credits", no_argument, NULL, 'c'}, 
            {"help", no_argument, NULL, 'h'}, 
            {"info", no_argument, NULL, 'i'},
            {NULL, 0, NULL, 0}
    };
    
    string optargSTR;
    double valDouble;
    
    try
    {
        while((opt = getopt_long(totalArgs, argv, shortOpts, longOpts, &longIndex)) != -1)
        {
            /*
             * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
             * Otherwise, it prevents the code from recognizing flags like -v, -h or -g (because 
             * they require no argument) and make it throw an exception... To avoid this, a second 
             * switch is used.
             *
             * (this is noteworthy, as this error is still present in ExploreNET v2.1)
             */
            
            switch(opt)
            {
                case 'a':
                case 'c':
                case 'e':
                case 'g':
                case 'h':
                case 'i':
                case 'n':
                case 'm':
                case 'o':
                case 'r':
                case 's':
                case 't':
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
                case 'm':
                    parsingOmitMerging = false;
                    break;
                case 'w':
                    gotNb = std::atoi(optargSTR.c_str());
                    if (gotNb > 2 && gotNb <= 20)
                    {
                        nbIPIDs = (unsigned short) gotNb;
                    }
                    else
                    {
                        cout << "Warning for -w option: a value outside the suggested range ";
                        cout << "(i.e., [3,20]) was provided. Architect will use the default ";
                        cout << "value for this option (= 4).\n" << endl;
                        break;
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
                        cout << "Warning for -x option: a value outside the suggested range ";
                        cout << "(i.e., [1,256]) was provided. Architect will use the default ";
                        cout << "value for this option (= 10).\n" << endl;
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
                        cout << "Warning for -y option: a negative value was provided. Architect ";
                        cout << "will use the default value for this option (= 0.2).\n" << endl;
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
                        cout << "Warning for -x option: a value outside the suggested range ";
                        cout << "(i.e., [0,0.5[) was provided. Architect will use the default ";
                        cout << "value for this option (= 0.35).\n" << endl;
                    }
                    break;
                case 'l':
                    labelOutputFiles = optargSTR;
                    break;
                case 's':
                    computeStatistics = true;
                    break;
                case 't':
                    outputTree = true;
                    break;
                case 'a':
                    outputNeighborhoods = true;
                    break;
                case 'o':
                    outputL2 = true;
                    break;
                case 'n':
                    createBipNS = true;
                    break;
                case 'e':
                    createBipERS = true;
                    break;
                case 'g':
                    createNGraph = true;
                    break;
                case 'r':
                    createRGraph = true;
                    break;
                case 'u':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb > 0 && gotNb <= 2)
                        createSGraph = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -u option: an unrecognized mode (i.e., value ";
                        cout << "out of [1,2]) was provided. Architect will use the first ";
                        cout << "mode (projection on subnets of NS graph).\n" << endl;
                    }
                    break;
                case 'v':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 1)
                        displayMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,1]) was provided. Architect will use the laconic ";
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
    }
    catch(std::logic_error &le)
    {
        cout << "Unrecognized option " << (char) opt << ". Halting now." << endl;
        cout << "Use -h or --help to get more details on how to use TreeNET." << endl;
        return 1;
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
    
    // Some checks to see if Architect as actually anything to do.
    if(!outputTree && !outputNeighborhoods && !outputL2 && !createBipERS && !createBipNS && 
       !createNGraph && !computeStatistics && !createRGraph && createSGraph == SUBNET_GRAPH_NONE)
    {
        cout << "No particular output was requested. Use -h or --help to get more details on how ";
        cout << "to use TreeNET \"Architect\"." << endl;
        return 0;
    }
    
    if(!found)
    {
        cout << "No input file was provided. Use -h or --help to get more details on how to use ";
        cout << "TreeNET \"Architect\"." << endl;
        return 0;
    }
    
    /*
     * SETTING THE ENVIRONMENT
     *
     * Before parsing the input dataset, the initialization of Architect is completed by creating 
     * a TreeNETEnvironment object, a singleton which will be passed by pointer to other classes 
     * of the program to be able to get all the current settings, which are either default values 
     * either values parsed in the parameters provided by the user. The singleton also provides 
     * access to data structures other classes should be able to access.
     */

    // Initialization of the environment
    TreeNETEnvironment *env = new TreeNETEnvironment(&cout, 
                                                     !parsingOmitMerging, 
                                                     nbIPIDs, 
                                                     maxRollovers, 
                                                     baseTolerance, 
                                                     maxError, 
                                                     displayMode);
    
    // Gets quick access to subnet set
    SubnetSiteSet *set = env->getSubnetSet();
    
    /*
     * If relevant (i.e. in case the user asked for statistics or a bipartite conversion), the 
     * label for the new output files is determined here for convenience. It is either the label 
     * selected by the user via -l flag, either the current time in the format dd-mm-YYYY hh:mm:ss.
     */
    
    string newFileName = "";
    if(labelOutputFiles.length() > 0)
    {
        newFileName = labelOutputFiles;
    }
    else
    {
        // Get the current time for the name of output files (if any)
        time_t rawTime;
        struct tm *timeInfo;
        char buffer[80];

        time(&rawTime);
        timeInfo = localtime(&rawTime);

        // Name for the output file containing the list of subnets
        strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
        string timeStr(buffer);
        newFileName = timeStr;
    }
    
    // Some welcome message
    cout << "TreeNET v3.2 \"Architect\" (time at start: " << getCurrentTimeStr() << ")\n" << endl;
    
    /*
     * INPUT DATASET PARSING
     *
     * The provided input dataset is read and parsed into the subnet set and the IP dictionnary 
     * of the current instance of TreeNET "Architect". The program stops if it spots a comma in 
     * the main argument, as it means the user is trying to input several datasets (which is 
     * possible in Forester, but not Architect).
     */
     
    cout << "--- Start of input file(s) parsing ---" << endl;
    timeval parsingStart, parsingEnd;
    gettimeofday(&parsingStart, NULL);

    size_t pos = inputsStr.find(',');
    if(pos != std::string::npos)
    {
        cout << "Architect is not meant to parse several datasets at once. Please use TreeNET ";
        cout << "\"Forester\" if you intend to merge several datasets together.\n" << endl;
        delete env;
        return 0;
    }
    
    string subnetDumpPath = inputsStr + ".subnet";
    
    SubnetParser *sp = new SubnetParser(env);
    bool subnetParsingResult = sp->parse(subnetDumpPath);
    delete sp;
    
    if(subnetParsingResult && set->getNbSubnets() > 0)
    {
        string ipDictPath = inputsStr + ".ip";
        
        IPDictionnaryParser *idp = new IPDictionnaryParser(env);
        bool ipParsingResult = idp->parse(ipDictPath);
        delete idp;
        
        if(!ipParsingResult)
        {
            env->fillIPDictionnary();
        }
    }
    else
    {
        cout << "Could not parse any subnet. Architect cannot continue and will halt now." << endl;
        delete env;
        return 0;
    }
    
    cout << "--- End of input file(s) parsing (" << getCurrentTimeStr() << ") ---" << endl;
    gettimeofday(&parsingEnd, NULL);
    unsigned long parsingElapsed = parsingEnd.tv_sec - parsingStart.tv_sec;
    cout << "Elapsed time: " << elapsedTimeStr(parsingElapsed) << "\n" << endl;

    /*
     * SUBNET NEIGHBORHOOD INFERENCE
     *
     * We now proceed to build the network tree in the same fashion as Arborist/Forester. 
     * However, in Architect, there is no preparation step at all. The code assumes the input 
     * dataset already provides all the useful data and goes straight to the network tree 
     * growth.
     */
    
    cout << "--- Start of neighborhood inference ---" << endl;
    timeval nInferenceStart, nInferenceEnd;
    gettimeofday(&nInferenceStart, NULL);
    
    Grower *g = new ClassicGrower(env);
    g->grow();
    
    Soil *result = g->getResult();
    delete g;
    
    // Tree output
    if(outputTree)
    {
        env->openLogStream(newFileName + ".tree");
        Climber *robin = new Robin(env);
        robin->climb(result);
        env->closeLogStream();
        delete robin;
        
        cout << "Network tree in text form has been written in a new file \"";
        cout << newFileName << ".tree\"." << endl;
    }
    
    cout << "--- End of neighborhood inference (" << getCurrentTimeStr() << ") ---" << endl;
    gettimeofday(&nInferenceEnd, NULL);
    unsigned long nInferenceElapsed = nInferenceEnd.tv_sec - nInferenceStart.tv_sec;
    cout << "Elapsed time: " << elapsedTimeStr(nInferenceElapsed) << "\n" << endl;
    
    /*
     * ALIAS RESOLUTION (AND ANALYSIS)
     *
     * The alias resolution can now take place. As Architect is entirely passive, only the actual 
     * alias resolution occurs (there is no hint collection). Indeed, it is assumed the provided 
     * dataset already provides all the data Architect needs. Just after, Architect performs 
     * tree/neighborhood/statistical analysis of the final tree before going to the conversion 
     * into (bipartite) graphs.
     */
     
    cout << "--- Start of alias resolution and tree analysis ---" << endl;
    timeval aliasResoStart, aliasResoEnd;
    gettimeofday(&aliasResoStart, NULL);
    
    // Re-does alias resolution
    Climber *crow = new Crow(env);
    crow->climb(result);
    
    if(outputNeighborhoods)
    {
        ((Crow*) crow)->outputAliases(newFileName + ".alias");
    
        env->openLogStream(newFileName + ".neighborhoods");
        Climber *cat = new Cat(env);
        cat->climb(result);
        env->closeLogStream();
        delete cat;
        
        cout << "New aliases have been written in a new file \"";
        cout << newFileName << ".aliases\".\n";
        cout << "Neighborhood analysis has been written in a new file \"";
        cout << newFileName << ".neighborhoods\".\n";
    }
    
    delete crow;
    
    if(outputL2)
    {
        env->openLogStream(newFileName + ".l2");
        Climber *termite = new Termite(env);
        termite->climb(result);
        env->closeLogStream();
        delete termite;
        
        cout << "L2 estimation (experimental) has been written in a new file \"";
        cout << newFileName << ".l2\".\n";
        
        // TODO in the future: takes account of L2 estimation in graph conversion
    }
    
    // Computation of statistics (if asked by the user)
    if(computeStatistics)
    {
        Ant *ant = new Ant(env);
        ant->climb(result);
        cout << ant->getStatisticsStr();
        ant->outputStatistics("Statistics " + newFileName);
        delete ant;
        
        cout << "\nYou will also find these statistics in a file ";
        cout << "\"Statistics " << newFileName << "\"." << endl;
    }
    
    cout << "--- End of alias resolution and analysis (" << getCurrentTimeStr() << ") ---" << endl;
    gettimeofday(&aliasResoEnd, NULL);
    unsigned long aliasResoElapsed = aliasResoEnd.tv_sec - aliasResoStart.tv_sec;
    cout << "Elapsed time: " << elapsedTimeStr(aliasResoElapsed) << "\n" << endl;

    // Graph generation
    
    if(createBipNS || createNGraph || createSGraph == SUBNET_GRAPH_FROM_NS)
    {
        cout << "--- Start of NS graph conversion ---" << endl;
        timeval NSGraphStart, NSGraphEnd;
        gettimeofday(&NSGraphStart, NULL);
    
        NSProcesser *NSTranslator = new NSProcesser(env);
        
        cout << "Processing the network tree into a neighborhood - subnet bipartite ";
        cout << "graph... " << flush;
        NSTranslator->process(result);
        cout << "Done." << endl;
        
        cout << "Checking that the graph has a single connected component... " << flush;
        double completeness = NSTranslator->check();
        cout << "Done." << endl;
        
        if(completeness == 100.0)
        {
            cout << "The graph is indeed made of a single component." << endl;
        }
        else
        {
            cout << "The graph has several components (main component = " << completeness;
            cout << "\% of the vertices)." << endl;
        }
        
        if(createNGraph)
        {
            NSTranslator->outputNeighborhoodProjection(newFileName + ".n-graph");
            cout << "Projection on neighborhoods has been written in a new file \"";
            cout << newFileName << ".n-graph\"." << endl;
        }
        
        if(createSGraph == SUBNET_GRAPH_FROM_NS)
        {
            NSTranslator->outputSubnetProjection(newFileName + ".s-graph");
            cout << "Projection on subnets has been written in a new file \"";
            cout << newFileName << ".s-graph\"." << endl;
        }
        
        if(createBipNS)
        {
            NSTranslator->output(newFileName + ".ns-graph");
            cout << "Full bipartite graph has been written in a new file \"";
            cout << newFileName << ".ns-graph\"." << endl;
        }
        
        delete NSTranslator;
        
        cout << "--- End of NS graph conversion (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&NSGraphEnd, NULL);
        unsigned long NSGraphElapsed = NSGraphEnd.tv_sec - NSGraphStart.tv_sec;
        cout << "Elapsed time: " << elapsedTimeStr(NSGraphElapsed) << "\n" << endl;
    }
    
    if(createBipERS || createRGraph || createSGraph == SUBNET_GRAPH_FROM_ERS)
    {
        cout << "--- Start of ERS graph conversion ---" << endl;
        timeval ERSGraphStart, ERSGraphEnd;
        gettimeofday(&ERSGraphStart, NULL);
    
        ERSProcesser *ERSTranslator = new ERSProcesser(env);
        
        cout << "Processing the network tree into an Ethernet switch - router - subnet double ";
        cout << "bipartite graph... " << flush;
        ERSTranslator->process(result);
        cout << "Done." << endl;
        
        cout << "Checking that the graph has a single connected component... " << flush;
        double completeness = ERSTranslator->check();
        cout << "Done." << endl;
        
        if(completeness == 100.0)
        {
            cout << "The graph is indeed made of a single component." << endl;
        }
        else
        {
            cout << "The graph has several components (main component = " << completeness;
            cout << "\% of the vertices)." << endl;
        }
        
        if(createRGraph)
        {
            ERSTranslator->outputRouterProjection(newFileName + ".r-graph");
            cout << "Projection on routers has been written in a new file \"";
            cout << newFileName << ".r-graph\"." << endl;
        }
        
        if(createSGraph == SUBNET_GRAPH_FROM_ERS)
        {
            ERSTranslator->outputSubnetProjection(newFileName + ".s-graph");
            cout << "Projection on subnets has been written in a new file \"";
            cout << newFileName << ".s-graph\"." << endl;
        }
        
        if(createBipERS)
        {
            ERSTranslator->output(newFileName + ".ers-graph");
            cout << "Full double bipartite graph has been written in a new file \"";
            cout << newFileName << ".ers-graph\"." << endl;
        }
        
        delete ERSTranslator;
        
        cout << "--- End of ERS graph conversion (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&ERSGraphEnd, NULL);
        unsigned long ERSGraphElapsed = ERSGraphEnd.tv_sec - ERSGraphStart.tv_sec;
        cout << "Elapsed time: " << elapsedTimeStr(ERSGraphElapsed) << "\n" << endl;
    }
    
    delete result;
    delete env;
    return 0;
}
