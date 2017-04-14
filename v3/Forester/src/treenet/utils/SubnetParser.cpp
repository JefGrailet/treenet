/*
 * SubnetParser.cpp
 *
 *  Created on: Jan 6, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetParser.h.
 */

#include <algorithm>
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;
#include <iomanip>
using std::setprecision;
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::endl;
using std::flush;

#include "SubnetParser.h"

SubnetParser::SubnetParser(TreeNETEnvironment *env)
{
    this->env = env;
    this->parsedSubnets = 0;
    this->credibleSubnets = 0;
    this->mergedSubnets = 0;
    this->duplicateSubnets = 0;
    this->badSubnets = 0;
}

SubnetParser::~SubnetParser()
{
}

bool SubnetParser::parse(string inputFileName)
{
    return this->parse(inputFileName, env->getSubnetSet());
}

bool SubnetParser::parse(string inputFileName, SubnetSiteSet *dest)
{
    ostream *out = env->getOutputStream();
    (*out) << "Parsing " << inputFileName << "..." << endl;
    
    // Resetting count fields for next parsing.
    this->parsedSubnets = 0;
    this->credibleSubnets = 0;
    this->mergedSubnets = 0;
    this->duplicateSubnets = 0;
    this->badSubnets = 0;

    string inputFileContent = "";
    ifstream inFile;
    inFile.open((inputFileName).c_str());
    if(inFile.is_open())
    {
        inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                (std::istreambuf_iterator<char>()));
        
        inFile.close();
    }
    else
    {
        (*out) << "File " << inputFileName << " does not exist.\n" << endl;
        return false;
    }
    
    this->parse(dest, inputFileContent);
    
    (*out) << "Parsing of " << inputFileName << " completed.\n" << flush;
    if(this->parsedSubnets == 0)
        (*out) << "No subnet was parsed. Associated .ip file will not be parsed.\n";
    (*out) << endl;
    
    return true;
}

void SubnetParser::parse(SubnetSiteSet *dest, string inputFileContent)
{
    ostream *out = env->getOutputStream();
    unsigned short displayMode = env->getDisplayMode();
    bool useMerging = env->usingMergingAtParsing();
    
    std::stringstream ss(inputFileContent);
    std::string targetStr;
    unsigned short nbLine = 0; // Counts the line while parsing
    bool ignoreTillBlankLine = false; // Ignores non-null lines until next blank line
    SubnetSite *temp = new SubnetSite(); // Temporar subnet
    
    while (std::getline(ss, targetStr, '\n'))
    {
        if(targetStr.size() == 0)
        {
            // Parsing went well: insertion can occur
            if((nbLine == 4 || nbLine == 5) && !ignoreTillBlankLine)
            {
                temp->completeRefinedData();
                
                if(!useMerging)
                {
                    string subnetStr = temp->getInferredNetworkAddressString();
                    
                    dest->addSiteNoMerging(temp);
                    
                    bool credibleSubnet = temp->isCredible();
                    this->parsedSubnets++;
                    if(credibleSubnet)
                        this->credibleSubnets++;
                    
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    {
                        (*out) << "New subnet: " << subnetStr;
                        if (credibleSubnet)
                            (*out) << " (credible) ";
                        (*out) << endl;
                    }
                }
                else
                {
                    unsigned short result = dest->addSite(temp);
                    string subnetStr = temp->getInferredNetworkAddressString();
                    
                    if(result == SubnetSiteSet::NEW_SUBNET)
                    {
                        bool credibleSubnet = temp->isCredible();
                        this->parsedSubnets++;
                        if(credibleSubnet)
                            this->credibleSubnets++;
                        
                        /*
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            (*out) << "New subnet: " << subnetStr;
                            if (credibleSubnet)
                                (*out) << " (credible) ";
                            (*out) << endl;
                        }
                        */
                        
                        // TreeNET (any version) no longer displays new correctly parsed subnets.
                    }
                    else if(result == SubnetSiteSet::KNOWN_SUBNET)
                    {
                        this->duplicateSubnets++;
                        
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            (*out) << "Duplicate subnet: " << subnetStr << endl;
                            delete temp;
                        }
                    }
                    else if(result == SubnetSiteSet::SMALLER_SUBNET)
                    {
                        this->mergedSubnets++;
                        
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            (*out) << "Merged with equivalent/larger subnet: " << subnetStr << endl;
                            delete temp;
                        }
                    }
                    else if(result == SubnetSiteSet::BIGGER_SUBNET)
                    {
                        this->mergedSubnets++;
                        
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                            (*out) << "Merged with smaller subnet(s): " << subnetStr << endl;
                    }
                }
                
                temp = new SubnetSite();
                nbLine = 0;
                ignoreTillBlankLine = false;
            }
            else
            {
                if(temp != NULL)
                    delete temp;
                temp = new SubnetSite();
                nbLine = 0;
                ignoreTillBlankLine = false;
            }
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
                    this->badSubnets++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        (*out) << "Malformed/Unrecognized subnet \"" + targetStr + "\"" << endl;
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
                    this->badSubnets++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        (*out) << "Malformed/Unrecognized subnet \"" + targetStr + "\"" << endl;
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
                    this->badSubnets++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    {
                        string subnetStr = temp->getInferredNetworkAddressString();
                        (*out) << "Unrecognized status \"" + targetStr + "\" for subnet ";
                        (*out) << subnetStr << endl;
                    }
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
                    std::string TTLStr = targetStr.substr(pos2 + 1);
                    
                    InetAddress liveIP(0);
                    try
                    {
                        liveIP.setInetAddress(IPStr);
                    }
                    catch (InetAddressException &e)
                    {
                        this->badSubnets++;
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            string subnetStr = temp->getInferredNetworkAddressString();
                            (*out) << "Malformed/Unrecognized interface \"" + targetStr;
                            (*out) << "\" in subnet " << subnetStr << "." << endl << "As ";
                            (*out) << "it is the only listed interface, this subnet will ";
                            (*out) << "not be further parsed." << endl;
                        }
                        ignoreTillBlankLine = true;
                        continue;
                    }
                    
                    unsigned char TTL = (unsigned char) std::atoi(TTLStr.c_str());
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
                    std::string TTLStr = nodeStr.substr(pos2 + 1);
                    
                    InetAddress liveIP(0);
                    try
                    {
                        liveIP.setInetAddress(IPStr);
                    }
                    catch (InetAddressException &e)
                    {
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            string subnetStr = temp->getInferredNetworkAddressString();
                            (*out) << "Malformed/Unrecognized interface \"" + nodeStr;
                            (*out) << "\" in subnet " << subnetStr << endl;
                        }
                        continue;
                    }
                    
                    unsigned char TTL = (unsigned char) std::atoi(TTLStr.c_str());
                    SubnetSiteNode *newNode = new SubnetSiteNode(liveIP, TTL);
                    temp->insert(newNode);
                    atLeastOne = true;
                }
                
                if(!atLeastOne)
                {
                    this->badSubnets++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    {
                        (*out) << "No correct interface was listed. This subnet will not ";
                        (*out) << "be further parsed." << endl;
                    }
                    ignoreTillBlankLine = true;
                    continue;
                }
            
                nbLine++;
            }
            // (post-processed) Route
            else if(nbLine == 3 || nbLine == 4)
            {
                if(targetStr.compare("No route") == 0)
                {
                    ignoreTillBlankLine = true;
                    continue;
                }
                
                // Case of line 4; it must be post-processed (starting with: "Post-processed: ")
                bool isPostProcessed = false;
                if(nbLine == 4)
                {
                    isPostProcessed = true;
                    string startStr = targetStr.substr(0, 16);
                    if(startStr.compare("Post-processed: ") == 0)
                    {
                        targetStr = targetStr.substr(16);
                    }
                    else
                    {
                        ignoreTillBlankLine = true;
                        continue;
                    }
                }
                
                size_t pos = targetStr.find(',');
                
                // Case where there is a single interface listed
                if(pos == std::string::npos)
                {
                    // Parses probe token and IP identifier if any
                    size_t pos2 = targetStr.find('[');
                    std::string IPStr = targetStr;
                    std::string infoStr = "";
                    if(pos2 != std::string::npos)
                    {
                        IPStr = targetStr.substr(0, pos2 - 1);
                        infoStr = targetStr.substr(pos2 + 1);
                        infoStr = infoStr.substr(0, -1);
                    }
                
                    InetAddress liveIP(0);
                    if(IPStr.compare("Missing") != 0 && IPStr.compare("Anonymous") != 0 && IPStr.compare("Skipped") != 0)
                    {
                        try
                        {
                            liveIP.setInetAddress(IPStr);
                        }
                        catch (InetAddressException &e)
                        {
                            this->badSubnets++;
                            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                            {
                                string subnetStr = temp->getInferredNetworkAddressString();
                                (*out) << "Malformed/Unrecognized interface \"" + targetStr;
                                (*out) << "\" in the route of subnet " << subnetStr << ". ";
                                (*out) << "As this is the only interface, it will not be ";
                                (*out) << "listed." << endl;
                            }
                            ignoreTillBlankLine = true;
                            continue;
                        }
                    }
                    
                    RouteInterface *route = new RouteInterface[1];
                    
                    /*
                     * N.B.: "Repaired-1", "Repaired" and "R" all denote the same operation. The 
                     * different names simply comes from different versions of TreeNET. The goal 
                     * is to allow parsing of old datasets.
                     */
                    
                    if(infoStr.length() > 0)
                    {
                        if(infoStr.compare("Repaired-1") == 0 || infoStr.compare("Repaired") == 0 || infoStr.compare("R") == 0)
                            route[0].repair(liveIP);
                        else if(infoStr.compare("Repaired-2") == 0)
                            route[0].repairBis(liveIP);
                        else if(infoStr.compare("Limited") == 0)
                            route[0].deanonymize(liveIP);
                        else if(infoStr.compare("Predicted") == 0 || infoStr.compare("P") == 0)
                            route[0].predict(liveIP);
                        else
                            route[0].update(liveIP);
                    }
                    else
                        route[0].update(liveIP);
                    
                    if(isPostProcessed)
                    {
                        temp->setProcessedRouteSize(1);
                        temp->setProcessedRoute(route);
                    }
                    else
                    {
                        temp->setRouteSize(1);
                        temp->setRoute(route);
                    }
                    nbLine++;
                    continue;
                }
                
                // Case of multiple interfaces being listed (separated by ", ")
                std::stringstream routeStream(targetStr);
                string routeStr;
                bool first = true, routeOK = true;
                list<RouteInterface> routeLs;
                
                while (std::getline(routeStream, routeStr, ','))
                {
                    // Avoids the space after the coma (except for the first interface)
                    if(!first)
                        routeStr = routeStr.substr(1);
                    else
                        first = false;
                    
                    // Parses IP plus an optional, additionnal string put between brackets
                    size_t pos2 = routeStr.find('[');
                    std::string IPStr = routeStr;
                    std::string infoStr = "";
                    if(pos2 != std::string::npos)
                    {
                        IPStr = routeStr.substr(0, pos2 - 1);
                        infoStr = routeStr.substr(pos2 + 1);
                        infoStr = infoStr.substr(0, -1);
                    }

                    InetAddress liveIP(0);
                    if(IPStr.compare("Missing") != 0 && IPStr.compare("Anonymous") != 0 && IPStr.compare("Skipped") != 0)
                    {
                        try
                        {
                            liveIP.setInetAddress(IPStr);
                        }
                        catch (InetAddressException &e)
                        {
                            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                            {
                                string subnetStr = temp->getInferredNetworkAddressString();
                                (*out) << "Malformed/Unrecognized interface \"" + routeStr;
                                (*out) << "\" in route to subnet " << subnetStr << endl;
                            }
                            routeOK = false;
                            break;
                        }
                    }
                    
                    RouteInterface newHop;
                    if(infoStr.length() > 0)
                    {
                        if(infoStr.compare("Repaired-1") == 0 || infoStr.compare("Repaired") == 0 || infoStr.compare("R") == 0)
                            newHop.repair(liveIP);
                        else if(infoStr.compare("Repaired-2") == 0)
                            newHop.repairBis(liveIP);
                        else if(infoStr.compare("Limited") == 0)
                            newHop.deanonymize(liveIP);
                        else if(infoStr.compare("Predicted") == 0 || infoStr.compare("P") == 0)
                            newHop.predict(liveIP);
                        else
                            newHop.update(liveIP);
                    }
                    else
                        newHop.update(liveIP);
                    routeLs.push_back(newHop);
                }
                
                if(!routeOK)
                {
                    this->badSubnets++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        (*out) << "Malformed route. This subnet will not be listed." << endl;
                    ignoreTillBlankLine = true;
                    continue;
                }
                
                // Saving route
                unsigned short routeSize = (unsigned short) routeLs.size();
                RouteInterface *route = new RouteInterface[routeSize];
                for(unsigned short i = 0; i < routeSize; i++)
                {
                    route[i] = routeLs.front();
                    routeLs.pop_front();
                }
                
                if(isPostProcessed)
                {
                    temp->setProcessedRouteSize(routeSize);
                    temp->setProcessedRoute(route);
                }
                else
                {
                    temp->setRouteSize(routeSize);
                    temp->setRoute(route);
                }
                nbLine++;
            }
        }
    }
    delete temp;
    
    // Summary of parsing
    if(this->parsedSubnets > 0)
    {
        (*out) << "Parsed subnets: " << this->parsedSubnets << endl;
        if(this->credibleSubnets > 0)
        {
            float ratio = ((float) this->credibleSubnets / (float) this->parsedSubnets) * 100;
            (*out) << "Credible subnets: " << this->credibleSubnets << " (";
            (*out) << setprecision(3) << ratio << "%)" << endl;
        }
    }
    if(this->duplicateSubnets > 0)
        (*out) << "Duplicate subnets: " << this->duplicateSubnets << endl;
    if(this->mergedSubnets > 0)
        (*out) << "Merged subnets: " << this->duplicateSubnets << endl;
    if(this->badSubnets > 0)
        (*out) << "Badly formatted subnets: " << this->badSubnets << endl;
}
