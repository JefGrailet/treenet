/*
 * SubnetParser.cpp
 *
 *  Created on: Jan 6, 2016
 *      Author: grailet
 *
 * Implements the class defined in SubnetParser.h.
 */

#include <algorithm>
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;

#include "SubnetParser.h"

SubnetParser::SubnetParser(TreeNETEnvironment *env)
{
    this->env = env;
}

SubnetParser::~SubnetParser()
{
}

void SubnetParser::parseInputFile(string inputFileContent, bool storeInIPDict)
{
    ostream *out = env->getOutputStream();
    SubnetSiteSet *set = env->getSubnetSet();
    bool setRefinement = env->getSetRefinement();
    
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
            if(nbLine == 3 && !ignoreTillBlankLine)
            {
                temp->completeRefinedData();
                
                if(!setRefinement)
                {
                    string subnetStr = temp->getInferredNetworkAddressString();
                    
                    set->addSiteNoRefinement(temp);
                    (*out) << "New subnet: " << subnetStr;
                    if (temp->isCredible())
                        (*out) << " (credible) ";
                    (*out) << endl;
                }
                else
                {
                    unsigned short result = set->addSite(temp);
                    string subnetStr = temp->getInferredNetworkAddressString();
                    
                    if(result == SubnetSiteSet::NEW_SUBNET)
                    {
                        (*out) << "New subnet: " << subnetStr;
                        if (temp->isCredible())
                            (*out) << " (credible) ";
                        (*out) << endl;
                    }
                    else if(result == SubnetSiteSet::KNOWN_SUBNET)
                    {
                        (*out) << "Known subnet: " << subnetStr << endl;
                        delete temp;
                    }
                    else if(result == SubnetSiteSet::SMALLER_SUBNET)
                    {
                        (*out) << "Merged with equivalent/larger subnet: " << subnetStr << endl;
                        delete temp;
                    }
                    else if(result == SubnetSiteSet::BIGGER_SUBNET)
                        (*out) << "Merged with smaller subnet(s): " << subnetStr << endl;
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
                    string subnetStr = temp->getInferredNetworkAddressString();
                    (*out) << "Unrecognized status \"" + targetStr + "\" for subnet " << subnetStr << endl;
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
                        string subnetStr = temp->getInferredNetworkAddressString();
                        (*out) << "Malformed/Unrecognized interface \"" + targetStr;
                        (*out) << "\" in subnet " << subnetStr << "." << endl << "As ";
                        (*out) << "it is the only listed interface, this subnet will ";
                        (*out) << "not be further parsed." << endl;
                        ignoreTillBlankLine = true;
                        continue;
                    }
                    
                    unsigned char TTL = (unsigned char) std::atoi(TTLStr.c_str());
                    SubnetSiteNode *newNode = new SubnetSiteNode(liveIP, TTL);
                    temp->insert(newNode);
                    
                    if(storeInIPDict)
                    {
                        IPTableEntry *entry = env->getIPTable()->create(liveIP);
                        if(entry != NULL)
                            entry->setTTL(TTL);
                    }
                    
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
                        string subnetStr = temp->getInferredNetworkAddressString();
                        (*out) << "Malformed/Unrecognized interface \"" + nodeStr;
                        (*out) << "\" in subnet " << subnetStr << endl;
                        continue;
                    }
                    
                    unsigned char TTL = (unsigned char) std::atoi(TTLStr.c_str());
                    SubnetSiteNode *newNode = new SubnetSiteNode(liveIP, TTL);
                    temp->insert(newNode);
                    
                    if(storeInIPDict)
                    {
                        IPTableEntry *entry = env->getIPTable()->create(liveIP);
                        if(entry != NULL)
                            entry->setTTL(TTL);
                    }
                    
                    atLeastOne = true;
                }
                
                if(!atLeastOne)
                {
                    (*out) << "No correct interface was listed. This subnet will not ";
                    (*out) << "be further parsed." << endl;
                    ignoreTillBlankLine = true;
                    continue;
                }
            
                nbLine++;
            }
            else if(nbLine == 3)
            {
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
                    try
                    {
                        liveIP.setInetAddress(IPStr);
                    }
                    catch (InetAddressException &e)
                    {
                        string subnetStr = temp->getInferredNetworkAddressString();
                        (*out) << "Malformed/Unrecognized interface \"" + targetStr;
                        (*out) << "\" in the route of subnet " << subnetStr << ".";
                        ignoreTillBlankLine = true;
                        continue;
                    }
                    
                    InetAddress *route = new InetAddress[1];
                    route[0] = liveIP;
                    temp->setRouteSize(1);
                    temp->setRoute(route);
                    
                    if(infoStr.compare("Repaired") == 0)
                    {
                        bool *repairMask = new bool[1];
                        repairMask[0] = true;
                        temp->setRouteRepairMask(repairMask);
                    }
                    
                    continue;
                }
                
                // Case of multiple interfaces being listed (separated by ", ")
                list<InetAddress> routeLs;
                list<bool> repairMaskLs;
                bool hasRepairedRoute = false;
                
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
                    try
                    {
                        liveIP.setInetAddress(IPStr);
                    }
                    catch (InetAddressException &e)
                    {
                        string subnetStr = temp->getInferredNetworkAddressString();
                        (*out) << "Malformed/Unrecognized interface \"" + routeStr;
                        (*out) << "\" in route to subnet " << subnetStr << endl;
                        routeOK = false;
                        break;
                    }
                    
                    routeLs.push_back(liveIP);
                    
                    if(infoStr.compare("Repaired") == 0)
                    {
                        repairMaskLs.push_back(true);
                        hasRepairedRoute = true;
                    }
                    else
                        repairMaskLs.push_back(false);
                }
                
                if(!routeOK)
                {
                    (*out) << "Malformed route. This subnet will not be listed." << endl;
                    ignoreTillBlankLine = true;
                    continue;
                }
                
                // Saving route
                unsigned short routeSize = (unsigned short) routeLs.size();
                InetAddress *route = new InetAddress[routeSize];
                for(unsigned short i = 0; i < routeSize; i++)
                {
                    route[i] = routeLs.front();
                    routeLs.pop_front();
                }
                
                temp->setRouteSize(routeSize);
                temp->setRoute(route);
                
                // Saving repair mask, if any
                if(hasRepairedRoute)
                {
                    bool *repairMask = new bool[routeSize];
                    for(unsigned short i = 0; i < routeSize; i++)
                    {
                        repairMask[i] = repairMaskLs.front();
                        repairMaskLs.pop_front();
                    }
                    temp->setRouteRepairMask(repairMask);
                }
            }
        }
    }
    delete temp;
}
