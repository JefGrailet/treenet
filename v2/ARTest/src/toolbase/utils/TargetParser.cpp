/*
 * TargetParser.cpp
 *
 *  Created on: Nov 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in TargetParser.h.
 */

#include <cstdlib>
#include <algorithm>
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;

#include "TargetParser.h"

TargetParser::TargetParser(TreeNETEnvironment *env)
{
    this->env = env;
}

TargetParser::~TargetParser()
{
}

void TargetParser::parseInputFile(string inputFileContent)
{
    if(inputFileContent.size() == 0)
    {
        return;
    }
    list<Router*> *routerList = env->getRouterList();
    
    stringstream ss1(inputFileContent);
    string targetStr1;
    
    while(getline(ss1, targetStr1, '\n'))
    {
        Router *newRouter = new Router();
        stringstream ss2(targetStr1);
        string targetStr2;
        
        while(getline(ss2, targetStr2, ' '))
        {
            InetAddress curIP(targetStr2);
            
            newRouter->addExpectedIP(curIP);
            parsedIPs.push_back(curIP);
        }
        
        routerList->push_back(newRouter);
    }
}

list<InetAddress> TargetParser::reorder(list<InetAddress> toReorder)
{
    unsigned long nbTargets = (unsigned long) toReorder.size();
    unsigned short maxThreads = env->getMaxThreads();
    list<InetAddress> reordered;
    
    // If there are more targets than the amount of threads which will be used
    if(nbTargets > (unsigned long) maxThreads)
    {
        /*
         * TARGET REORDERING
         *
         * As targets are usually given in order, probing consecutive targets may be 
         * inefficient as targetted interfaces may not be as responsive if multiple 
         * threads probe them at the same time (because of the traffic generated at a 
         * time), making inference results less complete and accurate. To avoid this, the 
         * list of targets is re-organized to avoid consecutive IPs as much as possible.
         */
        
        list<InetAddress>::iterator it = toReorder.begin();
        for(unsigned long i = 0; i < nbTargets; i++)
        {
            reordered.push_back((*it));
            toReorder.erase(it--);
            
            for(unsigned short j = 0; j < maxThreads; j++)
            {
                it++;
                if(it == toReorder.end())
                    it = toReorder.begin();
            }
        }
    }
    else
    {
        // In this case, we will just shuffle the list.
        std::vector<InetAddress> targetsV(toReorder.size());
        std::copy(toReorder.begin(), toReorder.end(), targetsV.begin());
        std::random_shuffle(targetsV.begin(), targetsV.end());
        std::list<InetAddress> shuffledTargets(targetsV.begin(), targetsV.end());
        reordered = shuffledTargets;
    }
    
    return reordered;
}

list<InetAddress> TargetParser::getTargets()
{
    NetworkAddress LAN = env->getLAN();
    InetAddress LANLowerBorder = LAN.getLowerBorderAddress();
    InetAddress LANUpperBorder = LAN.getUpperBorderAddress();
    list<InetAddress> targets;
    
    for(std::list<InetAddress>::iterator it = parsedIPs.begin(); it != parsedIPs.end(); ++it)
    {
        InetAddress target((*it));
        
        if(target >= LANLowerBorder && target <= LANUpperBorder)
            continue;
        
        targets.push_back(target);
    }
    
    return reorder(targets);
}

bool TargetParser::targetsInLAN()
{
    NetworkAddress LAN = env->getLAN();
    InetAddress LANLowerBorder = LAN.getLowerBorderAddress();
    InetAddress LANUpperBorder = LAN.getUpperBorderAddress();
    
    for(std::list<InetAddress>::iterator it = parsedIPs.begin(); it != parsedIPs.end(); ++it)
    {
        if((*it) >= LANLowerBorder && (*it) <= LANUpperBorder)
            return true;
    }
    return false;
}
