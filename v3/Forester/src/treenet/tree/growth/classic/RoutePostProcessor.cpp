/*
 * RoutePostProcessor.cpp
 *
 *  Created on: Mar 27, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in RoutePostProcessor.h (see this class for more details).
 */

#include <list>
using std::list;

#include <map>
using std::map;
using std::pair;

#include "RoutePostProcessor.h"

RoutePostProcessor::RoutePostProcessor(TreeNETEnvironment *env)
{
    this->env = env;
    
    printSteps = false;
    if(env->getDisplayMode() >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        printSteps = true;
}

RoutePostProcessor::~RoutePostProcessor()
{
}

void RoutePostProcessor::process()
{
    this->detect();
    this->mitigate();
}

void RoutePostProcessor::detect()
{
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnets = env->getSubnetSet();
    IPLookUpTable *dict = env->getIPTable();
    list<SubnetSite*> *ssList = subnets->getSubnetSiteList();
    
    // Evaluates route stretching
    unsigned short totalAffectedSubnets = 0;
    (*out) << "Evaluating route stretching..." << endl;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        unsigned short nbStretches = 0;
        unsigned short maxStretch = 0;
        if(curSubnet->hasValidRoute())
        {
            unsigned short routeSize = curSubnet->getRouteSize();
            RouteInterface *route = curSubnet->getRoute();
            for(unsigned short i = 0; i < routeSize; i++)
            {
                InetAddress hop = route[i].ip;
                if(hop == InetAddress(0))
                    continue;
                
                IPTableEntry *ipEntry = dict->lookUp(hop);
                if(ipEntry == NULL) // Should not occur, but just in case, if it does, we skip
                    continue;
                
                unsigned short shortestTTL = (unsigned short) ipEntry->getTTL();
                if((i + 1) > shortestTTL)
                {
                    unsigned short diff = (i + 1) - shortestTTL;
                    route[i].state = RouteInterface::STRETCHED;
                    
                    nbStretches++;
                    if(diff > maxStretch)
                        maxStretch = diff;
                }
            }
            
            if(nbStretches > 0)
            {
                totalAffectedSubnets++;
                if(nbStretches > 1)
                    (*out) << "Found " << nbStretches << " stretches";
                else
                    (*out) << "Found one stretch";
                (*out) << " in route to " << curSubnet->getInferredNetworkAddressString() << " (";
                if(nbStretches > 1)
                    (*out) << "longest ";
                if(maxStretch > 1)
                    (*out) << "stretch: " << maxStretch << " hops)." << endl;
                else
                    (*out) << "stretch: " << maxStretch << " hop)." << endl;
            }
        }
    }
    if(totalAffectedSubnets > 0)
        (*out) << "Done.\n" << endl;
    else
        (*out) << "No router suffers from route stretching.\n" << endl;
    
    // Evaluates route cycling
    totalAffectedSubnets = 0;
    (*out) << "Evaluating route cycling..." << endl;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        if(curSubnet->hasValidRoute())
        {
            this->checkForCycles(curSubnet);
            unsigned short longestCycle = 0;
            unsigned short nbCycles = this->countCycles(curSubnet, &longestCycle);
            if(nbCycles > 0)
            {
                totalAffectedSubnets++;
                if(nbCycles > 1)
                    (*out) << "Found " << nbCycles << " cycles";
                else
                    (*out) << "Found one cycle";
                (*out) << " in route to " << curSubnet->getInferredNetworkAddressString() << " (";
                if(nbCycles > 1)
                    (*out) << "maximum ";
                (*out) << "cycle length: " << longestCycle << ")." << endl;
            }
        }
    }
    if(totalAffectedSubnets > 0)
        (*out) << "Done.\n" << endl;
    else
        (*out) << "No router suffers from route cycling.\n" << endl;
}

void RoutePostProcessor::mitigate()
{
    ostream *out = env->getOutputStream();
    SubnetSiteSet *set = env->getSubnetSet();
    list<SubnetSite*> *ssList = set->getSubnetSiteList();

    /*
     * Step 1: preparation
     * -------------------
     *
     * In order to avoid useless processing in subsequent steps, the method first lists the 
     * subnets which need post-processing. In the route to each, the stretched IPs are also listed 
     * in order to map afterwards each of them with a subnet that contains them (when it exists) 
     * because it is possible the soonest occurrence (in TTL) of an IP occurs in its subnet.
     */
    
    list<SubnetSite*> subnets;
    map<InetAddress, SubnetSite*> mapSubnets;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        if(curSubnet->hasValidRoute())
        {
            if(this->needsPostProcessing(curSubnet))
            {
                subnets.push_back(curSubnet);
                
                unsigned short routeSize = curSubnet->getRouteSize();
                RouteInterface *route = curSubnet->getRoute();
                for(unsigned short i = 0; i < routeSize; i++)
                {
                    if(route[i].state == RouteInterface::STRETCHED)
                    {
                        InetAddress curIP = route[i].ip;
                        SubnetSite *container = set->getSubnetContaining(curIP);
                        if(container != NULL && container->hasValidRoute())
                            mapSubnets.insert(pair<InetAddress, SubnetSite*>(curIP, container));
                    }
                }
            }
        }
    }
    
    // Checks if there are routes to post-process at all.
    size_t nbRoutesToFix = subnets.size();
    if(nbRoutesToFix > 0)
    {
        if(nbRoutesToFix > 1)
            (*out) << "There are " << nbRoutesToFix << " routes to post-process." << endl;
        else
            (*out) << "There is one route to post-process." << endl;
    }
    else
    {
        return;
    }
    
    /*
     * Step 2: last hop correction
     * ---------------------------
     */
    
    unsigned short lastHopFixed = 0;
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *subnet = (*it);
        string subnetStr = subnet->getInferredNetworkAddressString();
        if(this->needsLastHopCorrection(subnet))
        {
            if(printSteps)
                (*out) << "Fixing last hop of route to " << subnetStr << "... " << std::flush;
        
            unsigned short routeSize = subnet->getRouteSize();
            RouteInterface *route = subnet->getRoute();
            
            unsigned short newRouteSize = routeSize - 1;
            RouteInterface *newRoute = new RouteInterface[newRouteSize];
            
            for(unsigned short i = 0; i < newRouteSize; ++i)
                newRoute[i] = route[i];
            
            subnet->setProcessedRouteSize(newRouteSize);
            subnet->setProcessedRoute(newRoute);
            
            lastHopFixed++;
            
            if(printSteps)
                (*out) << "Done." << endl;
            
            // If no other needs, we remove this subnet from the list for next steps
            if(!this->needsCyclingMitigation(subnet) && !this->needsStretchingMitigation(subnet))
            {
                subnets.erase(it--);
            }
        }
    }
    if(lastHopFixed > 0)
    {
        (*out) << "Fixed last hop on ";
        if(lastHopFixed > 1)
            (*out) << lastHopFixed << " routes.";
        else
            (*out) << "one route.";
        (*out) << endl;
    }
    
    /*
     * Step 3: route cycling mitigation
     * --------------------------------
     */
    
    unsigned short cycleFixed = 0;
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *subnet = (*it);
        string subnetStr = subnet->getInferredNetworkAddressString();
        if(this->needsCyclingMitigation(subnet))
        {
            bool firstRouteMeasured = true;
            if(subnet->getProcessedRouteSize() > 0 && subnet->getProcessedRoute() != NULL)
                firstRouteMeasured = false;
            
            unsigned short prevRouteSize = 0, newRouteSize = 0;
            RouteInterface *prevRoute = NULL, *newRoute = NULL;
            if(firstRouteMeasured)
            {
                prevRouteSize = subnet->getRouteSize();
                prevRoute = subnet->getRoute();
            }
            else
            {
                prevRouteSize = subnet->getProcessedRouteSize();
                prevRoute = subnet->getProcessedRoute();
            }
            
            if(printSteps)
            {
                (*out) << "Mitigating cycle(s) in route to " << subnetStr;
                (*out) << " (" << prevRouteSize << " hops)..." << endl;
            }
            
            // As long as there remains cycle(s) in the route...
            while(this->hasCycle(prevRoute, prevRouteSize))
            {
                // Starts from the end to find a cycle
                unsigned short cycleStart = 0, cycleEnd = 0;
                for(short int i = prevRouteSize - 1; i >= 0; i--)
                {
                    if(prevRoute[i].state == RouteInterface::CYCLE)
                    {
                        cycleEnd = i;
                        break;
                    }
                }
                InetAddress cycledIP = prevRoute[cycleEnd].ip;
                
                if(cycleEnd == 0) // Just in case
                    break;
                
                if(printSteps)
                {
                    (*out) << "Found a cycled IP (" << cycledIP << ") at TTL = ";
                    (*out) << (cycleEnd + 1) << "." << endl;
                }
                
                // Gets start of the cycle
                for(short int i = cycleEnd - 1; i >= 0; i--)
                    if(prevRoute[i].ip == prevRoute[cycleEnd].ip)
                        cycleStart = i;
                
                if(printSteps)
                {
                    (*out) << "Found start of the cycle at TTL = ";
                    (*out) << (cycleEnd + 1) << "." << endl;
                }
                
                // Lists hops before and after the cycle
                list<RouteInterface> newHops;
                for(unsigned short i = 0; i < cycleStart; i++)
                    newHops.push_back(prevRoute[i]);
                for(unsigned short i = cycleEnd; i < prevRouteSize; i++)
                    newHops.push_back(prevRoute[i]);
                
                // Creates the new route
                unsigned short index = 0;
                newRouteSize = (unsigned short) newHops.size();
                newRoute = new RouteInterface[newRouteSize];
                
                if(printSteps)
                    (*out) << "New route size is " << newRouteSize << " hops." << endl;
                
                while(newHops.size() > 0)
                {
                    newRoute[index] = newHops.front();
                    newHops.pop_front();
                    if(newRoute[index].ip == cycledIP)
                        newRoute[index].state = RouteInterface::VIA_TRACEROUTE;
                    index++;
                }
                
                // If the "prevRoute" is initially the measured route, we should NOT delete it
                if(firstRouteMeasured)
                    firstRouteMeasured = false;
                else
                    delete[] prevRoute;
                
                prevRouteSize = newRouteSize;
                prevRoute = newRoute;
            }
            
            if(newRoute != NULL)
            {
                cycleFixed++;
                subnet->setProcessedRouteSize(newRouteSize);
                subnet->setProcessedRoute(newRoute);
            }
            
            if(printSteps)
                (*out) << "Done with route to " << subnetStr << "." << endl;
            
            // If no other needs, we remove this subnet from the list for the last step
            if(!this->needsStretchingMitigation(subnet))
            {
                subnets.erase(it--);
            }
        }
    }
    if(cycleFixed > 0)
    {
        (*out) << "Mitigated route cycling on ";
        if(cycleFixed > 1)
            (*out) << cycleFixed << " routes.";
        else
            (*out) << "one route.";
        (*out) << endl;
    }
    
    /*
     * Step 4: route stretching mitigation
     * -----------------------------------
     */
    
    unsigned short stretchFixed = 0;
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *subnet = (*it);
        string subnetStr = subnet->getInferredNetworkAddressString();
        
        // Here, there should be no need to check that the route needs mitigation.
        
        bool firstRouteMeasured = true;
        if(subnet->getProcessedRouteSize() > 0 && subnet->getProcessedRoute() != NULL)
            firstRouteMeasured = false;
        
        unsigned short prevRouteSize = 0, newRouteSize = 0;
        RouteInterface *prevRoute = NULL, *newRoute = NULL;
        if(firstRouteMeasured)
        {
            prevRouteSize = subnet->getRouteSize();
            prevRoute = subnet->getRoute();
        }
        else
        {
            prevRouteSize = subnet->getProcessedRouteSize();
            prevRoute = subnet->getProcessedRoute();
        }
        
        if(printSteps)
        {
            (*out) << "Mitigating stretch(es) in route to " << subnetStr;
            (*out) << " (" << prevRouteSize << " hops)..." << endl;
        }
        
        // As long as there remains stretch(es) in the route...
        while(this->hasStretch(prevRoute, prevRouteSize))
        {
            // Starts from the end to find a stretched IP
            unsigned short stretchOffset = 0;
            for(short int i = prevRouteSize - 1; i >= 0; i--)
            {
                if(prevRoute[i].state == RouteInterface::STRETCHED)
                {
                    stretchOffset = i;
                    break;
                }
            }
            InetAddress IPToFix = prevRoute[stretchOffset].ip;
            
            if(stretchOffset == 0) // Just in case
                continue;
            
            if(printSteps)
            {
                (*out) << "Found a stretched IP (" << IPToFix << ") at TTL = ";
                (*out) << (stretchOffset + 1) << "." << endl;
            }
            
            // Now looks for the route prefix of the earliest occurrence of that IP
            unsigned short prefixSize = 0;
            RouteInterface *prefix = this->findPrefix(IPToFix, &prefixSize);
            if(prefix == NULL)
            {
                // Looks up in mapSubnets (earliest occurrence might be in a subnet)
                map<InetAddress, SubnetSite*>::iterator res = mapSubnets.find(IPToFix);
                if(res != mapSubnets.end())
                {
                    SubnetSite *container = res->second;
                    
                    // Last check: is the TTL strictly shorter ?
                    if(container->getShortestTTL() < (unsigned char) (stretchOffset + 1))
                    {
                        prefixSize = container->getRouteSize();
                        prefix = container->getRoute();
                        
                        if(prefixSize == 0 || prefix == NULL)
                        {
                            if(printSteps)
                            {
                                (*out) << "Missing route prefix upon seeking it in another ";
                                (*out) << "subnet. No more post-processing for this route." << endl;
                                break;
                            }
                        }
                        
                        if(container->getStatus() == SubnetSite::ACCURATE_SUBNET || 
                           container->getStatus() == SubnetSite::ODD_SUBNET)
                        {
                            /*
                             * Last thing to do: if earliest occurrence is a contra-pivot of an 
                             * accurate or odd subnet, prefixSize should be decremented.
                             */
                            
                            list<SubnetSiteNode*> *nodes = container->getSubnetIPList();
                            for(list<SubnetSiteNode*>::iterator it2 = nodes->begin(); it2 != nodes->end(); ++it2)
                            {
                                SubnetSiteNode *cur = (*it2);
                                if(cur->ip == IPToFix)
                                {
                                    if(cur->TTL == container->getShortestTTL())
                                    {
                                        prefixSize--;
                                        break;
                                    }
                                }
                            }
                        }
                        
                        if(printSteps)
                        {
                            (*out) << "Found new route prefix (length = " << prefixSize;
                            (*out) << " hops) in another subnet." << endl;
                        }
                    }
                    else
                    {
                        if(printSteps)
                            (*out) << "There is no better prefix (false positive)." << endl;
                        prevRoute[stretchOffset].state = RouteInterface::VIA_TRACEROUTE;
                        break;
                    }
                }
                // Because it cannot find soonest occurrence and its prefix, code gets out of loop.
                else
                {
                    if(printSteps)
                        (*out) << "There is no better prefix (false positive)." << endl;
                    prevRoute[stretchOffset].state = RouteInterface::VIA_TRACEROUTE;
                    break;
                }
            }
            else
            {
                if(printSteps)
                {
                    (*out) << "Found new route prefix (length = " << prefixSize;
                    (*out) << " hops) in other routes." << endl;
                }
            }
            
            // Lists hops before and after the stretch
            list<RouteInterface> newHops;
            for(unsigned short i = 0; i < prefixSize; i++)
                newHops.push_back(prefix[i]);
            for(unsigned short i = stretchOffset; i < prevRouteSize; i++)
                newHops.push_back(prevRoute[i]);
            
            // Creates the new route
            unsigned short index = 0;
            newRouteSize = (unsigned short) newHops.size();
            newRoute = new RouteInterface[newRouteSize];
            
            if(printSteps)
                (*out) << "New route length is " << newRouteSize << " hops." << endl;
            
            while(newHops.size() > 0)
            {
                newRoute[index] = newHops.front();
                newHops.pop_front();
                if(newRoute[index].ip == IPToFix)
                    newRoute[index].state = RouteInterface::VIA_TRACEROUTE;
                index++;
            }
            
            // If the "prevRoute" is initially the measured route, we should NOT delete it
            if(firstRouteMeasured)
                firstRouteMeasured = false;
            else
                delete[] prevRoute;
            
            prevRouteSize = newRouteSize;
            prevRoute = newRoute;
        }
        
        if(newRoute != NULL)
        {
            stretchFixed++;
            subnet->setProcessedRouteSize(newRouteSize);
            subnet->setProcessedRoute(newRoute);
        }
        
        if(printSteps)
            (*out) << "Done with route to " << subnetStr << "." << endl;
    }
    if(stretchFixed > 0)
    {
        (*out) << "Mitigated route stretching on ";
        if(stretchFixed > 1)
            (*out) << stretchFixed << " routes.";
        else
            (*out) << "one route.";
        (*out) << endl;
    }
    
    (*out) << endl;
}

void RoutePostProcessor::checkForCycles(SubnetSite *ss)
{
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();
    for(unsigned short i = 0; i < routeSize; ++i)
    {
        if(route[i].state == RouteInterface::CYCLE || route[i].ip == InetAddress(0))
            continue;
    
        InetAddress curStep = route[i].ip;
        for(unsigned short j = i + 1; j < routeSize; ++j)
        {
            InetAddress nextStep = route[j].ip;
            if(curStep == nextStep)
                route[j].state = RouteInterface::CYCLE;
        }
    }
}

unsigned short RoutePostProcessor::countCycles(SubnetSite *ss, unsigned short *longest)
{
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();
    
    unsigned short nbCycles = 0, longestCycle = 0;
    bool toIgnore[routeSize];
    for(unsigned short i = 0; i < routeSize; i++)
        toIgnore[i] = false;
    
    for(short int i = routeSize - 1; i >= 0; --i)
    {
        if(toIgnore[i])
            continue;
    
        if(route[i].state == RouteInterface::CYCLE)
        {
            nbCycles++;
            InetAddress curIP = route[i].ip;
            unsigned short cycleLength = 0;
            for(short int j = i - 1; j >= 0; --j)
            {
                if(route[j].ip == curIP)
                {
                    cycleLength = i - j;
                    toIgnore[j] = true;
                }
            }
            
            if(cycleLength > longestCycle)
                longestCycle = cycleLength;
        }
    }
    (*longest) = longestCycle;
    return nbCycles;
}

bool RoutePostProcessor::needsPostProcessing(SubnetSite *ss)
{
    unsigned short status = ss->getStatus();
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();

    if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
    {
        InetAddress lastIP = route[routeSize - 1].ip;
        if(ss->contains(lastIP))
        {
            /*
             * Last check: lastIP should be a regular pivot IP to justify the fix. If it is a 
             * contra-pivot IP, of the subnet, then there is no problem with that.
             */
            
            unsigned char cpTTL = ss->getShortestTTL();
            list<SubnetSiteNode*> *nodes = ss->getSubnetIPList();
            for(list<SubnetSiteNode*>::iterator it = nodes->begin(); it != nodes->end(); ++it)
                if((*it)->ip == lastIP && (*it)->TTL > cpTTL)
                    return true;
            
            return false;
        }
    }

    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].state == RouteInterface::CYCLE || route[i].state == RouteInterface::STRETCHED)
            return true;
    
    return false;
}

bool RoutePostProcessor::needsLastHopCorrection(SubnetSite *ss)
{
    unsigned short status = ss->getStatus();
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();

    if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
    {
        InetAddress lastIP = route[routeSize - 1].ip;
        if(ss->contains(lastIP))
        {
            /*
             * Last check: lastIP should be a regular pivot IP to justify the fix. If it is a 
             * contra-pivot IP, of the subnet, then there is no problem with that.
             */
            
            unsigned char cpTTL = ss->getShortestTTL();
            list<SubnetSiteNode*> *nodes = ss->getSubnetIPList();
            for(list<SubnetSiteNode*>::iterator it = nodes->begin(); it != nodes->end(); ++it)
                if((*it)->ip == lastIP && (*it)->TTL > cpTTL)
                    return true;
            
            return false;
        }
    }
    
    return false;
}

bool RoutePostProcessor::needsCyclingMitigation(SubnetSite *ss)
{
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();

    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].state == RouteInterface::CYCLE)
            return true;
    
    return false;
}

bool RoutePostProcessor::needsStretchingMitigation(SubnetSite *ss)
{
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();

    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].state == RouteInterface::STRETCHED)
            return true;
    
    return false;
}

bool RoutePostProcessor::hasCycle(RouteInterface *route, unsigned short size)
{
    for(unsigned short i = 0; i < size; ++i)
        if(route[i].state == RouteInterface::CYCLE)
            return true;
    
    return false;
}

bool RoutePostProcessor::hasStretch(RouteInterface *route, unsigned short size)
{
    for(unsigned short i = 0; i < size; ++i)
        if(route[i].state == RouteInterface::STRETCHED)
            return true;
    
    return false;
}

RouteInterface* RoutePostProcessor::findPrefix(InetAddress stretched, unsigned short *size)
{
    SubnetSiteSet *set = env->getSubnetSet();
    IPLookUpTable *dict = env->getIPTable();
    list<SubnetSite*> *ssList = set->getSubnetSiteList();
    
    // 1) Finds shortest TTL seen for this hop
    IPTableEntry *hop = dict->lookUp(stretched);
    if(hop == NULL)
        return NULL;
    
    unsigned char TTL = hop->getTTL();
    
    // 2) Inspects the routes of all subnets at that given hop
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        if(curSubnet->hasValidRoute() && curSubnet->getRouteSize() >= (unsigned short) TTL)
        {
            RouteInterface *route = curSubnet->getRoute();
            if(route[(unsigned short) TTL - 1].ip == stretched)
            {
                (*size) = (unsigned short) TTL - 1;
                return route;
            }
        }
    }
    
    return NULL;
}
