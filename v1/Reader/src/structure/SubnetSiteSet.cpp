/*
 * SubnetSiteSet.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: grailet
 *
 * Implements the class defined in SubnetSiteSet.h (see this file to learn further about the 
 * goals of such class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include "SubnetSiteSet.h"
#include "../common/inet/NetworkAddress.h"

using namespace std;

SubnetSiteSet::SubnetSiteSet()
{
}

SubnetSiteSet::~SubnetSiteSet()
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        delete (*i);
    }
    siteList.clear();
}

bool SubnetSiteSet::isCompatible(InetAddress lowerBorder, InetAddress upperBorder)
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss2 = (*i);
        InetAddress lowerBorder2, upperBorder2;
        unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
        
        if(prefixLength2 <= 31)
        {
            NetworkAddress na2 = ss2->getInferredNetworkAddress();
            lowerBorder2 = na2.getLowerBorderAddress();
            upperBorder2 = na2.getUpperBorderAddress();
        }
        // /32 subnets
        else
        {
            lowerBorder2 = ss2->getPivotAddress();
            upperBorder2 = ss2->getPivotAddress();
        }
        
        if(lowerBorder2 <= lowerBorder)
        {
            // ss2 contains hypothetical subnet or is equivalent
            if(upperBorder2 >= upperBorder)
            {
                return false;
            }
            // upperBorder2 < upperBorder but lowerBorder == lowerBorder2: hypothetical subnet contains ss2
            else if(lowerBorder2 == lowerBorder)
            {
                return false;
            }
            // upperBorder2 < upperBorder: ss2 does not contain hypothetical subnet
            else
            {
                continue;
            }
        }
        // lowerBorder2 > lowerBorder
        else
        {
            // ss contains ss2
            if(upperBorder >= upperBorder2)
            {
                return false;
            }
            // upperBorder < upperBorder2: hypothetical subnet does not contain ss2
            else
            {
                continue;
            }
        }
    }
    
    return true;
}

unsigned short SubnetSiteSet::addSite(SubnetSite *ss)
{
    InetAddress lowerBorder1, upperBorder1;
    unsigned char prefixLength = ss->getInferredSubnetPrefixLength();
    if(prefixLength <= 31)
    {
        NetworkAddress na1 = ss->getInferredNetworkAddress();
        lowerBorder1 = na1.getLowerBorderAddress();
        upperBorder1 = na1.getUpperBorderAddress();
    }
    // /32 subnets
    else
    {
        lowerBorder1 = ss->getPivotAddress();
        upperBorder1 = ss->getPivotAddress();
    }
    
    unsigned short toReturn = SubnetSiteSet::NEW_SUBNET; // Default
    
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss2 = (*i);
        InetAddress lowerBorder2, upperBorder2;
        unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
        
        if(prefixLength2 <= 31)
        {
            NetworkAddress na2 = ss2->getInferredNetworkAddress();
            lowerBorder2 = na2.getLowerBorderAddress();
            upperBorder2 = na2.getUpperBorderAddress();
        }
        // /32 subnets
        else
        {
            lowerBorder2 = ss2->getPivotAddress();
            upperBorder2 = ss2->getPivotAddress();
        }
        
        if(lowerBorder2 <= lowerBorder1)
        {
            // ss2 contains ss or is equivalent
            if(upperBorder2 >= upperBorder1)
            {
                // Special case: identical /32 subnets
                if(prefixLength == 32 && prefixLength2 == 32)
                {
                    return SubnetSiteSet::KNOWN_SUBNET;
                }
            
                // Gets IPs in ss that are not in ss2 yet
                list<SubnetSiteNode*> *IPs1 = ss->getSubnetIPList();
                list<SubnetSiteNode*> *IPs2 = ss2->getSubnetIPList();
                unsigned int newInterfaces = 0;
                for(std::list<SubnetSiteNode*>::iterator it = IPs1->begin(); it != IPs1->end(); ++it)
                {
                    SubnetSiteNode *cur = *it;
                    bool found = false;
                    for(std::list<SubnetSiteNode*>::iterator it2 = IPs2->begin(); it2 != IPs2->end(); ++it2)
                    {
                        SubnetSiteNode *cur2 = *it2;
                        if(cur->ip == cur2->ip)
                        {
                            newInterfaces++;
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                        IPs2->push_back(cur);
                    else
                        delete cur;
                    IPs1->erase(it--);
                }
                IPs1->clear();
                IPs2->sort(SubnetSiteNode::smaller);

                toReturn = SubnetSiteSet::KNOWN_SUBNET;
                if(newInterfaces > 0)
                    toReturn = SubnetSiteSet::SMALLER_SUBNET;
                return toReturn;
            }
            // upperBorder2 < upperBorder1 but lowerBorder1 == lowerBorder2: ss contains ss2
            else if(lowerBorder2 == lowerBorder1)
            {
                // Gets IPs in ss2 that are not in ss yet
                list<SubnetSiteNode*> *IPs1 = ss->getSubnetIPList();
                list<SubnetSiteNode*> *IPs2 = ss2->getSubnetIPList();
                for(std::list<SubnetSiteNode*>::iterator it = IPs2->begin(); it != IPs2->end(); ++it)
                {
                    SubnetSiteNode *cur = *it;
                    bool found = false;
                    for(std::list<SubnetSiteNode*>::iterator it2 = IPs1->begin(); it2 != IPs1->end(); ++it2)
                    {
                        SubnetSiteNode *cur2 = *it2;
                        if(cur->ip == cur2->ip)
                        {
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                        IPs1->push_back(cur);
                    else
                        delete cur;
                    IPs2->erase(it--);
                }
                IPs2->clear();
                IPs1->sort(SubnetSiteNode::smaller);
                
                delete (*i);
                siteList.erase(i--);
                
                // We do not stop here, in case there was another subnet contained in ss.
                toReturn = SubnetSiteSet::BIGGER_SUBNET;
            }
            // upperBorder2 < upperBorder1: ss2 does not contain ss
            else
            {
                continue;
            }
        }
        // lowerBorder2 > lowerBorder1
        else
        {
            // ss contains ss2: removes registered subnet and put the new one
            if(upperBorder1 >= upperBorder2)
            {
                // Gets IPs in ss2 that are not in ss yet
                list<SubnetSiteNode*> *IPs1 = ss->getSubnetIPList();
                list<SubnetSiteNode*> *IPs2 = ss2->getSubnetIPList();
                for(std::list<SubnetSiteNode*>::iterator it = IPs2->begin(); it != IPs2->end(); ++it)
                {
                    SubnetSiteNode *cur = *it;
                    bool found = false;
                    for(std::list<SubnetSiteNode*>::iterator it2 = IPs1->begin(); it2 != IPs1->end(); ++it2)
                    {
                        SubnetSiteNode *cur2 = *it2;
                        if(cur->ip == cur2->ip)
                        {
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                        IPs1->push_back(cur);
                    else
                        delete cur;
                    IPs2->erase(it--);
                }
                IPs2->clear();
                IPs1->sort(SubnetSiteNode::smaller);
                
                delete (*i);
                siteList.erase(i--);
                
                // We do not stop here, in case there was another subnet contained in ss.
                toReturn = SubnetSiteSet::BIGGER_SUBNET;
            }
            // upperBorder1 < upperBorder2: ss does not contain ss2
            else
            {
                continue;
            }
        }
    }
    
    // Inserts ss and sorts the set
    siteList.push_back(ss);
    siteList.sort(SubnetSite::compare);
    return toReturn;
}

void SubnetSiteSet::addSiteNoRefinement(SubnetSite *ss)
{
    siteList.push_back(ss);
}

void SubnetSiteSet::sortSet()
{
    siteList.sort(SubnetSite::compare);
}

unsigned short SubnetSiteSet::getLongestRoute()
{
    unsigned short longest = 0;
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        if(ss->getRouteSize() > longest)
        {
            longest = ss->getRouteSize();
        }
    }
    return longest;
}

void SubnetSiteSet::sortByRoute()
{
    siteList.sort(SubnetSite::compareRoutes);
}

SubnetSite *SubnetSiteSet::getValidSubnet()
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        if(ss->getStatus() != SubnetSite::UNDEFINED_SUBNET)
        {
            siteList.erase(i--);
            return ss;
        }
    }
    return NULL;
}

void SubnetSiteSet::outputAsFile(string filename)
{
    string output = "";
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        string cur = ss->toString();
        
        if(!cur.empty())
            output += cur + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
