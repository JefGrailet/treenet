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

#include "utils/OutputHandler.h"
#include "utils/TreeNETEnvironment.h"
#include "utils/SubnetParser.h"
#include "utils/IPDictionnaryParser.h"
#include "utils/VantagePointSelector.h"

#include "paristraceroute/ParisTracerouteTask.h"
#include "aliasresolution/AliasResolver.h"

#include "prober/structure/ProbeRecord.h"
#include "prober/icmp/DirectICMPProber.h"

#include "structure/SubnetSite.h"
#include "structure/SubnetSiteSet.h"
#include "structure/NetworkTree.h"

#include "bipartite/BipartiteGraph.h"

int main(int argc, char *argv[])
{
    OutputHandler *outHandler = new OutputHandler(&cout);
    
    InetAddress localIPAddress;
    unsigned short probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_ICMP;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@ulg.ac.be)");
    string inputFilePath;
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND); // 2s + 500 000 microseconds = 2,5s
    TimeVal probeRegulatingPeriod(0, 50000); // 0,05s
    TimeVal probeThreadDelay(0, 250000); // 0,25s
    unsigned short nbIPIDs = 4; // For Alias Resolution
    unsigned short maxRollovers = 10; // Idem
    double baseTolerance = 0.2; // Idem
    double maxError = 0.35; // Idem
    unsigned short nbThreads = 256;
    string labelOutputFiles = "";
    bool doubleProbe = false; // TODO: add it in parameters
    bool useFixedFlowID = true;
    bool analyzeNeighborhoods = false;
    bool generateBipartite = false;
    bool computeStatistics = false;
    bool setRefinement = true;
    bool debugMode = false;
    
    /*
     * "Re-computation mode": integer value telling whether we are recomputing some data and output
     * files to improve their results. The possibilities are:
     * -0: only the input data will be used,
     * -1: available data is used to produce new .alias/.fingerprint files,
     * -2: alias resolution hints (and only them) are re-computed to replace the available data,
     * -3: both routes and alias resolution hints are re-computed to replace the available data,
     * -Above 3: not used.
     * Note that in the case of 2 and 3, .alias/.fingerprint files are also produced again.
     */
    
    const static unsigned short NO_RECOMPUTATION = 0;
    const static unsigned short RECOMPUTE_AR_RESULTS = 1;
    const static unsigned short RECOMPUTE_AR_HINTS = 2;
    const static unsigned short RECOMPUTE_ROUTES_AR = 3;
    unsigned short recomputationMode = 0;

    int opt = 0;
    int longIndex = 0;
    
    const char* const shortOpts = "i:e:u:m:f:w:z:t:d:j:x:c:y:l:r:a:snbgv?";
    const struct option longOpts[] = {
            {"input-file", required_argument, 0, 'i'}, 
            {"interface", required_argument, 0, 'e'}, 
            {"probing-protocol", required_argument, 0, 'u'}, 
            {"attention-message", required_argument, 0, 'm'}, 
            {"fix-flow-id", required_argument, 0, 'f'}, 
            {"probe-timeout-period", required_argument, 0, 'w'}, 
            {"probe-regulating-period", required_argument, 0, 'z'}, 
            {"probe-thread-delay", required_argument, 0, 'd'}, 
            {"amount-ip-ids", required_argument, 0, 'j'}, 
            {"max-rollovers", required_argument, 0, 'x'}, 
            {"base-tolerance", required_argument, 0, 'c'}, 
            {"max-error", required_argument, 0, 'y'}, 
            {"concurrency-nb-threads", required_argument, 0, 't'}, 
            {"label-output-files", required_argument, 0, 'l'}, 
            {"recomputation-mode", required_argument, NULL, 'r'}, 
            {"set-refinement", required_argument, 0, 'a'}, 
            {"statistics", no_argument, NULL, 's'}, 
            {"neighborhoods", no_argument, NULL, 'n'}, 
            {"bipartite", no_argument, NULL, 'b'}, 
            {"debug", no_argument, NULL, 'g'}, 
            {"version", no_argument, NULL, 'v'}, 
            {"help", no_argument, NULL, '?'}
    };

    // User arguments
    string optargSTR;
    unsigned long val;
    unsigned long sec;
    unsigned long microSec;
    double valDouble;
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
            case 'n':
            case 'b':
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
            case 'c':
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
            case 'l':
                labelOutputFiles = optargSTR;
                break;
            case 'r':
                gotNb = std::atoi(optargSTR.c_str());
                if (gotNb > 0 && gotNb < 3)
                {
                    recomputationMode = (unsigned short) gotNb;
                }
                break;
            case 'a':
                std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                if(optargSTR == string("FALSE"))
                    setRefinement = false;
                break;
            case 's':
                computeStatistics = true;
                break;
            case 'n':
                analyzeNeighborhoods = true;
                break;
            case 'b':
                generateBipartite = true;
                break;
            case 'g':
                debugMode = true;
                break;
            case 'v':
                cout << "TreeNET Reader v2.3 (" << string(argv[0]) << ") was written by J.-F. Grailet (2016)\n";
                cout << "Based on ExploreNET version 2.1 Copyright (c) 2013 Mehmet Engin Tozal" << endl;
                delete outHandler;
                return 0;
            case '?':
                outHandler->usage(string(argv[0]));
                delete outHandler;
                return 0;
            default:
                outHandler->usage(string(argv[0]));
                delete outHandler;
                return 1;
        }
    }

    // Determines local IP
    if(localIPAddress.isUnset() && recomputationMode != NO_RECOMPUTATION)
    {
        try
        {
            localIPAddress = InetAddress::getFirstLocalAddress();
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain a valid local IP address for probing." << endl;
            return 1;
        }
    }
    
    // Initialization of the environment
    TreeNETEnvironment *env = new TreeNETEnvironment(&cout, 
                                                     setRefinement, 
                                                     probingProtocol, 
                                                     doubleProbe, 
                                                     useFixedFlowID, 
                                                     localIPAddress, 
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
    
    SubnetSiteSet *set = env->getSubnetSet();
    
    // Following vars are declared here to ease memory freeing upon cathing SocketException
    unsigned short sizeParisArray = 0;
    Thread **parisTh = NULL;
    NetworkTree *tree = NULL;
    AliasHintCollector *ahc = NULL;
    
    try
    {
        // Some welcome message
        cout << "Welcome to TreeNET Reader\n" << endl;
        
        /*
         * INPUT FILE PARSING
         *
         * The provided input file(s) (either a single subnet dump, either a pair subnet dump/IP 
         * dictionnary dump) are read and parsed into the subnet set and the IP dictionnary of the 
         * current instance of TreeNET Reader.
         */
        
        if(inputFilePath.size() == 0)
        {
            cout << "Please input TreeNET dump files to continue.\n" << endl;
            throw InvalidParameterException();
        }
        
        cout << "Parsing input file(s)...\n" << endl;
    
        // Listing input dump file(s)
        bool mergingMode = false; // Triggered when multiple files are input
        list<string> filePaths; // Needs to be declared here for later
        
        size_t pos = inputFilePath.find(',');
        if(pos != std::string::npos)
        {
            // Listing all file paths
            std::stringstream ss(inputFilePath);
            std::string inputFileStr;
            while (std::getline(ss, inputFileStr, ','))
                filePaths.push_back(inputFileStr);
        
            // Checking that each file exists
            for(list<string>::iterator it = filePaths.begin(); it != filePaths.end(); ++it)
            {
                // Subnet dump
                ifstream inFile;
                inFile.open(((*it) + ".subnet").c_str());
                if(inFile.is_open())
                {
                    inFile.close();
                }
                else
                {
                    cout << "No " << (*it) << ".subnet to parse.\n" << endl;
                    filePaths.erase(it--);
                }
            }
            
            if(filePaths.size() > 1)
            {
                mergingMode = true;
            }
            else if(filePaths.size() == 1)
            {
                cout << "Multiple input files were provided, but only one of them actually ";
                cout << "exists in the file system. There will be no merging.\n" << endl;
                inputFilePath = filePaths.front();
            }
            else
            {
                cout << "Please input existing TreeNET dump files (minus .subnet or .ip ";
                cout << "extension) to continue.\n" << endl;
                throw InvalidParameterException();
            }
        }
        
        /*
         * If relevant (i.e. in case of route/alias resolution hints re-collection or merging 
         * mode), the label for the new output files is determined here for convenience. It is 
         * either the label selected by the user via -l flag, either the current time in the 
         * format dd-mm-YYYY hh:mm:ss.
         */
        
        string newFileName = "";
        if(generateBipartite || recomputationMode != 0 || mergingMode)
        {
            // Save the subnets with the recomputed routes.
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
        }
        
        // Single input file: both .subnet and .ip dumps are parsed
        if(!mergingMode)
        {
            // Parsing utilities
            SubnetParser *sp = new SubnetParser(env);
            IPDictionnaryParser *idp = new IPDictionnaryParser(env);
        
            // Subnet dump
            string subnetDumpContent = "";
            ifstream inFileSubnet;
            inFileSubnet.open((inputFilePath + ".subnet").c_str());
            bool subnetDumpOk = false;
            if(inFileSubnet.is_open())
            {
                subnetDumpContent.assign((std::istreambuf_iterator<char>(inFileSubnet)),
                                         (std::istreambuf_iterator<char>()));
                
                inFileSubnet.close();
                subnetDumpOk = true;
            }
            else
            {
                cout << "No " << inputFilePath << ".subnet to parse.\n" << endl;
            }
            
            if(subnetDumpOk)
            {
                // IP dictionnary dump
                string IPDumpContent = "";
                ifstream inFileIP;
                inFileIP.open((inputFilePath + ".ip").c_str());
                bool IPDumpOk = false;
                if(inFileIP.is_open())
                {
                    IPDumpContent.assign((std::istreambuf_iterator<char>(inFileIP)),
                                         (std::istreambuf_iterator<char>()));
                    
                    inFileIP.close();
                    IPDumpOk = true;
                }
                
                // Code also checks the amount of subnets before/after parsing
                size_t curNbSubnets = set->getSubnetSiteList()->size();
                cout << "Parsing " << inputFilePath << ".subnet..." << endl;
                sp->parse(set, subnetDumpContent, !IPDumpOk);
                cout << "Parsing of " << inputFilePath << ".subnet completed.\n" << endl;
                size_t newNbSubnets = set->getSubnetSiteList()->size();
                
                if(newNbSubnets > curNbSubnets)
                {
                    cout << "Parsing " << inputFilePath << ".ip..." << endl;
                    idp->parse(IPDumpContent);
                    cout << "Parsing of " << inputFilePath << ".ip completed.\n" << endl;
                }
                else if(IPDumpOk)
                {
                    cout << "No subnet were found. " << inputFilePath << ".ip parsing has been ";
                    cout << "skipped.\n" << endl;
                }
                else
                {
                    cout << "No " << inputFilePath << ".ip to parse. IP Dictionnary has been ";
                    cout << "partially completed with the IPs mentioned in ";
                    cout << inputFilePath << ".subnet.\n" << endl;
                }
            }
            
            delete sp;
            delete idp;
        }
        else
        {
            /*
             * MERGING MODE
             *
             * When several files are recognized, TreeNET Reader enter this mode to properly merge 
             * the different pieces of data together. Indeed, it is very likely these pieces were 
             * collected from distinct vantage points (or VPs), and therefore, their routes are 
             * not suited for building a whole tree with all sets together. To overcome this 
             * issue, TreeNET Reader v2.1+ introduces a "transplant" operation which predicts the 
             * route that would be obtained from a particular VP (selected via the specific class 
             * VantagePointSelection) to some subnet, relying on the last interfaces occurring in 
             * its route. The intuition is that, no matter the VP, the last interface(s) towards 
             * a subnet should stay the same.
             */
             
            cout << "Multiple input files were recognized. TreeNET Reader will now proceed to ";
            cout << "merge them together into a single dataset.\n" << endl;
            
            // Vantage point selection. Beginning of the full tree is built as well.
            VantagePointSelector *vps = new VantagePointSelector(env, filePaths);
            vps->select();
            NetworkTree *tree = vps->getStartTree();
            delete vps;
            
            // Starts the proper merging
            SubnetSiteSet *mainSet = env->getSubnetSet();
            SubnetSite *toInsert = mainSet->getValidSubnet();
            list<SubnetSite*> scraps;
            while(toInsert != NULL)
            {
                bool fitting = tree->fittingRoute(toInsert);
                if(!fitting)
                {
                    unsigned short oldPrefixSize = 0, newPrefixSize = 0;
                    InetAddress *oldPrefix = NULL, *newPrefix = NULL;
                    
                    bool transplant = tree->findTransplantation(toInsert,
                                                                &oldPrefixSize,
                                                                &oldPrefix,
                                                                &newPrefixSize,
                                                                &newPrefix);
                    
                    if(transplant)
                    {
                        unsigned short res = 1;
                        toInsert->transplantRoute(oldPrefixSize, newPrefixSize, newPrefix);
                        res += mainSet->transplantRoutes(oldPrefixSize, 
                                                         oldPrefix, 
                                                         newPrefixSize, 
                                                         newPrefix);
                        
                        // Prints out the transplantation that occurred
                        cout << "Transplantation:\nOriginal: ";
                        for(unsigned short i = 0; i < oldPrefixSize; i++)
                        {
                            if(i > 0)
                                cout << ", ";
                            cout << oldPrefix[i];
                        }
                        cout << "\nPredicted: ";
                        for(unsigned short i = 0; i < newPrefixSize; i++)
                        {
                            if(i > 0)
                                cout << ", ";
                            cout << newPrefix[i];
                        }
                        cout << "\n";
                        
                        if(res > 1)
                            cout << res << " transplantations.\n";
                        else
                            cout << "One transplantation.\n";
                        cout << endl;
                        
                        tree->insert(toInsert);
                    }
                    else
                    {
                        scraps.push_back(toInsert);
                    }
                }
                else
                {
                    tree->insert(toInsert);
                }
            
                toInsert = mainSet->getValidSubnet();
                if(toInsert == NULL)
                    toInsert = mainSet->getValidSubnet(false);
            }
            
            // Displays the tree itself
            tree->visit(&cout);
            cout << endl;
            
            // Saves the new data
            tree->outputSubnets(newFileName + ".subnet");
            cout << "Merged data has been saved in an output file " << newFileName;
            cout << ".subnet.\n" << endl;
            
            // Takes care of "scrap" subnets (i.e. subnets neither inserted, neither transplanted)
            if(scraps.size() > 0)
            {
                SubnetSiteSet *tempSet = new SubnetSiteSet();
                for(list<SubnetSite*>::iterator it = scraps.begin(); it != scraps.end(); ++it)
                {
                    tempSet->addSiteNoRefinement((*it));
                }
                tempSet->outputAsFile("Scraps " + newFileName);
                cout << "Scrap subnets (i.e., they could not be transplanted) have been saved ";
                cout << "in an output file Scraps " << newFileName << ".\n" << endl;
                delete tempSet;
            }
            
            cout << "Merging ended. TreeNET Reader will stop running here.\n\n";
            cout << "It is strongly recommended to re-run TreeNET Reader with the new dataset ";
            cout << "as input to re-collect alias resolution hints before analyzing the data.";
            cout << endl;
            
            // Stops here
            delete env;
            delete tree;
            delete outHandler;
            return 1;
        }
        
        // Stops and throws an exception if no valid subnet
        std::list<SubnetSite*> *list = set->getSubnetSiteList();
        if(list->size() == 0)
            throw InvalidParameterException();
        
        cout << "Parsing completed.\n" << endl;
        
        /*
         * PARIS TRACEROUTE
         *
         * If explicitely asked, we will re-compute the route to each subnet.
         */
        
        if(recomputationMode == RECOMPUTE_ROUTES_AR && list->size() > 0)
        {
            cout << "Recomputing route to each parsed subnet...\n" << endl;
            
            // Copying list of subnets
            std::list<SubnetSite*> toSchedule;
            for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
                toSchedule.push_back((*it));
            
            // Size of the thread array
            unsigned short maxThreads = nbThreads;
            if((unsigned long) toSchedule.size() > (unsigned long) maxThreads)
                sizeParisArray = maxThreads;
            else
                sizeParisArray = (unsigned short) toSchedule.size();
            
            // Creates thread(s)
            parisTh = new Thread*[sizeParisArray];
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
                    parisTh[i] = new Thread(new ParisTracerouteTask(env, 
                                                                    &subnetsToDelete, 
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
            
            // First save of the subnets with the new routes (in case tree building fails)
            set->outputAsFile(newFileName + ".subnet");
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
        
        tree = new NetworkTree(treeMaxDepth);
        
        // First, subnets with a complete route.
        SubnetSite *toInsert = set->getValidSubnet();
        while(toInsert != NULL)
        {
            tree->insert(toInsert);
            toInsert = set->getValidSubnet();
        }
        
        cout << "Subnets with complete route inserted." << endl;
        cout << "Now repairing incomplete routes to insert remaining subnets..." << endl;
        
        // Then, subnets with an incomplete route after a repairment
        toInsert = set->getValidSubnet(false);
        while(toInsert != NULL)
        {
            tree->repairRoute(toInsert);
            tree->insert(toInsert);
            toInsert = set->getValidSubnet(false);
        }
        cout << "Building complete.\n" << endl;
        
        // Final save of the subnets (if routes were previously re-computed).
        if(recomputationMode == RECOMPUTE_ROUTES_AR)
        {
            tree->outputSubnets(newFileName + ".subnet");
            cout << "Parsed subnets with successfully re-computed routes have been saved in an ";
            cout << "output file " << newFileName << ".subnet.\n" << endl;
        }
        
        // Displays the tree itself
        tree->visit(&cout);
        cout << endl;
        
        if(recomputationMode == RECOMPUTE_AR_HINTS || recomputationMode == RECOMPUTE_ROUTES_AR)
        {
            /*
             * ALIAS RESOLUTION
             *
             * Interfaces bordering a neighborhood are probed once more to attempt to gather 
             * interfaces as routers.
             */
            
            cout << "Alias resolution...\n" << endl;
            
            ahc = new AliasHintCollector(env);
            tree->collectAliasResolutionHints(&cout, ahc);
            cout << endl;
            delete ahc;

            env->getIPTable()->outputDictionnary(newFileName + ".ip");
            cout << "IP dictionnary with the new alias resolution hints has been saved in an ";
            cout << "output file " << newFileName << ".ip.\n" << endl;
        }
        
        // Internal node exploration is only done now.
        AliasResolver *ar = new AliasResolver(env);
        tree->inferRouters(ar);
        delete ar;
        
        // Largest Neighborhood
        cout << "Largest amount of fingerprints in a Neighborhood: ";
        cout << tree->largestFingerprintList();
        cout << "\n" << endl;
        
        // Outputs new .alias and .fingerprint files if necessary
        if(recomputationMode == RECOMPUTE_AR_RESULTS || 
           recomputationMode == RECOMPUTE_AR_HINTS || 
           recomputationMode == RECOMPUTE_ROUTES_AR)
        {
            tree->outputAliases(newFileName + ".alias");
            cout << "Inferred alias lists have been saved in an output file ";
            cout << newFileName << ".alias.\n";
        
            env->getIPTable()->outputFingerprints(newFileName + ".fingerprint");
            cout << "IP dictionnary with fingerprints has been saved in an output file ";
            cout << newFileName << ".fingerprint." << endl;
        }
        
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
            tree->internals(&cout);
        }
        
        if(generateBipartite)
        {
            cout << "Creating bipartite graph...\n" << endl;
            
            // Generates bipartite graph
            BipartiteGraph *graph = tree->toBipartite();
            
            // File name
            string bipFileName = "Bipartite " + newFileName;
            
            // Outputs the bipartite graph
            ofstream newFile;
            newFile.open(bipFileName.c_str());
            newFile << graph->routersToString() << endl;
            newFile << graph->subnetsToString() << endl;
            newFile << graph->linksSRToString() << endl;
            newFile << graph->linksRSToString() << endl;
            newFile.close();
            
            string newFilePath = "./" + bipFileName;
            chmod(newFilePath.c_str(), 0766);
            
            cout << "Bipartite graph outputted as " << bipFileName << ".\n" << endl;
            
            delete graph;
        }
        
        // Deletes tree
        delete tree;
        
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

        delete env;
        delete outHandler;
    }
    catch(SocketException &e)
    {
        cout << "\nUnable to create sockets to recompute routes and alias data. Try running ";
        cout << "TreeNET Reader as a privileged user (for example, try with sudo)." << endl;
        
        if(parisTh != NULL)
        {
            for(unsigned int i = 0; i < sizeParisArray; i++)
            {
                if(parisTh[i] != NULL)
                    delete parisTh[i];
            }
            delete[] parisTh;
        }
        
        if(tree != NULL)
            delete tree;
        
        if(ahc != NULL)
            delete ahc;
        
        delete env;
        delete outHandler;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "No valid subnet to work with.\n";
        cout << "Use \"--help\" or \"-?\" parameter to reach help." << endl;
        
        delete env;
        delete outHandler;
        return 1;
    }
    
    return 0;
}
