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

#include "SubnetParser.h"

SubnetParser::SubnetParser(TreeNETEnvironment *env)
{
    this->env = env;
}

SubnetParser::~SubnetParser()
{
}

void SubnetParser::parse(SubnetSiteSet *dest, string inputFileContent, bool storeInIPDict)
{
    ostream *out = env->getOutputStream();
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
            if(nbLine == 3 && !ignoreTillBlankLine)
            {
                temp->completeRefinedData();
                
                if(!useMerging)
                {
                    string subnetStr = temp->getInferredNetworkAddressString();
                    
                    dest->addSiteNoMerging(temp);
                    (*out) << "New subnet: " << subnetStr;
                    if (temp->isCredible())
                        (*out) << " (credible) ";
                    (*out) << endl;
                }
                else
                {
                    unsigned short result = dest->addSite(temp);
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
                if(targetStr.compare("No route") == 0)
                {
                    ignoreTillBlankLine = true;
                    continue;
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
                    if(IPStr.compare("Missing") != 0)
                    {
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
                    }
                    
                    RouteInterface *route = new RouteInterface[1];
                    if(infoStr.compare("Repaired") == 0 || infoStr.compare("R") == 0)
                        route[0].repair(liveIP);
                    //else if(infoStr.compare("P") == 0) TODO: put this back
                    //    route[0].predicts(liveIP);
                    else
                        route[0].update(liveIP);
                    temp->setRouteSize(1);
                    temp->setRoute(route);
                    
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
                    if(IPStr.compare("Missing") != 0)
                    {
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
                    }
                    
                    RouteInterface newHop;
                    if(infoStr.compare("Repaired") == 0 || infoStr.compare("R") == 0)
                        newHop.repair(liveIP);
                    else
                        newHop.update(liveIP);
                    routeLs.push_back(newHop);
                    
                    /*
                    TODO: put this back later
                    else if(infoStr.compare("P") == 0)
                    {
                        editMaskLs.push_back(SubnetSite::PREDICTED_INTERFACE);
                        hasEditedRoute = true;
                    }
                    */
                }
                
                if(!routeOK)
                {
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
                
                temp->setRouteSize(routeSize);
                temp->setRoute(route);
            }
        }
    }
    delete temp;
}
