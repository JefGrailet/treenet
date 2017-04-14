/*
 * ParisTracerouteTask.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in ParisTracerouteTask.h (see this file to learn further about the 
 * goals of such class).
 */

#include "ParisTracerouteTask.h"

ParisTracerouteTask::ParisTracerouteTask(TreeNETEnvironment *e, 
                                         list<SubnetSite*> *td, 
                                         SubnetSite *ss, 
                                         unsigned short lbii, 
                                         unsigned short ubii, 
                                         unsigned short lbis, 
                                         unsigned short ubis):
env(e), 
toDelete(td), 
subnet(ss)
{
    try
    {
        unsigned short protocol = env->getProbingProtocol();
    
        if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_UDP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env->usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectUDPWrappedICMPProber(env->getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    env->getTimeoutPeriod(), 
                                                    env->getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env->debugMode());
        }
        else if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_TCP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env->usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectTCPWrappedICMPProber(env->getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    env->getTimeoutPeriod(), 
                                                    env->getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env->debugMode());
        }
        else
        {
            prober = new DirectICMPProber(env->getAttentionMessage(), 
                                          env->getTimeoutPeriod(), 
                                          env->getProbeRegulatingPeriod(), 
                                          lbii, 
                                          ubii, 
                                          lbis, 
                                          ubis, 
                                          env->debugMode());
        }
    }
    catch(SocketException e)
    {
        ostream *out = env->getOutputStream();
        TreeNETEnvironment::consoleMessagesMutex.lock();
        (*out) << "Caught an exception because no new socket could be opened." << endl;
        TreeNETEnvironment::consoleMessagesMutex.unlock();
        this->stop();
        throw;
    }
    
    // Verbosity/debug stuff
    displayFinalRoute = false; // Default
    debugMode = false; // Default
    unsigned short displayMode = env->getDisplayMode();
    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        displayFinalRoute = true;
    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_DEBUG)
        debugMode = true;
    
    // Start of the new log with first probing details (debug mode only)
    if(debugMode)
    {
        this->log += "Computing route to " + subnet->getInferredNetworkAddressString() + "...\n";
        this->log += prober->getAndClearLog();
    }
}

ParisTracerouteTask::~ParisTracerouteTask()
{
    if(prober != NULL)
    {
        env->updateProbeAmounts(prober);
        delete prober;
    }
}

ProbeRecord *ParisTracerouteTask::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    ProbeRecord *record = NULL;
    
    try
    {
        if(env->usingDoubleProbe())
            record = prober->doubleProbe(localIP, dst, TTL, true);
        else
            record = prober->singleProbe(localIP, dst, TTL, true);
    }
    catch(SocketException e)
    {
        throw;
    }
    
    // Debug log
    if(debugMode)
    {
        this->log += prober->getAndClearLog();
    }
    
    /*
     * N.B.: Fixed flow is ALWAYS used in this case, otherwise this is regular Traceroute and not 
     * "Paris" Traceroute.
     */

    return record;
}

void ParisTracerouteTask::abort()
{
    this->toDelete->push_back(subnet);
    
    ostream *out = env->getOutputStream();
    TreeNETEnvironment::consoleMessagesMutex.lock();
    
    // This message is displayed anyway.
    (*out) << "Unable to recompute the route to ";
    (*out) << subnet->getInferredNetworkAddressString();
    (*out) << ". This subnet will be ignored." << endl << endl;
    
    TreeNETEnvironment::consoleMessagesMutex.unlock();
}

void ParisTracerouteTask::stop()
{
    TreeNETEnvironment::emergencyStopMutex.lock();
    env->triggerStop();
    TreeNETEnvironment::emergencyStopMutex.unlock();
}

void ParisTracerouteTask::run()
{
    // Picks up to five pivot addresses for destination
    list<InetAddress> candidatesDst = subnet->getPivotAddresses(ParisTracerouteTask::MAX_PIVOT_CANDIDATES);
    if(candidatesDst.size() == 0)
    {
        this->abort();
        return;
    }
    InetAddress probeDst = candidatesDst.front();
    candidatesDst.pop_front();
    
    // Initial timeout being used
    TimeVal usedTimeout = prober->getTimeout();
    
    // First checks the IP is responsive
    ProbeRecord *firstProbe = NULL;
    try
    {
        firstProbe = this->probe(probeDst, ParisTracerouteTask::MAX_HOPS);
    }
    catch(SocketException e)
    {
        this->stop();
        return;
    }
    
    while(firstProbe->getRplyICMPtype() != DirectProber::ICMP_TYPE_ECHO_REPLY && candidatesDst.size() > 0)
    {
        delete firstProbe;
        
        // Retries with another Pivot address
        probeDst = candidatesDst.front();
        candidatesDst.pop_front();
        try
        {
            firstProbe = this->probe(probeDst, ParisTracerouteTask::MAX_HOPS);
        }
        catch(SocketException e)
        {
            this->stop();
            return;
        }
    }
    
    // Last probe is still a failure: aborts
    if(firstProbe->getRplyICMPtype() != DirectProber::ICMP_TYPE_ECHO_REPLY)
    {
        delete firstProbe;
        this->abort();
        return;
    }
    
    // Otherwise it's OK. First probe is deleted and code proceeds with actual traceroute.
    delete firstProbe;
    
    /*
     * Then, we estimate the TTL required to get an ECHO reply from this destination and list the 
     * interfaces appearing at each hop.
     */
    
    unsigned char probeTTL = 1;
    list<InetAddress> interfaces; // List of interfaces (in order: TTL = 1, TTL = 2, etc.)
    while(probeTTL < ParisTracerouteTask::MAX_HOPS)
    {
        ProbeRecord *newProbe = NULL;
        try
        {
            newProbe = this->probe(probeDst, probeTTL);
        }
        catch(SocketException e)
        {
            this->stop();
            return;
        }
        InetAddress rplyAddress = newProbe->getRplyAddress();
    
        // Gets out of loop when we get an echo reply (means we reached destination)
        if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY) 
        {
            if(probeTTL == 1)
                interfaces.push_back(newProbe->getRplyAddress());
            delete newProbe;
            break;
        }
        
        // Ensures we have something different from 0.0.0.0
        if(rplyAddress == InetAddress(0))
        {
            delete newProbe;
            
            // New probe with twice the timeout period
            prober->setTimeout(usedTimeout * 2);
            try
            {
                newProbe = this->probe(probeDst, probeTTL);
            }
            catch(SocketException e)
            {
                this->stop();
                return;
            }
            rplyAddress = newProbe->getRplyAddress();
            
            // If still nothing different than 0.0.0.0, last try with 4 times the timeout
            if(rplyAddress == InetAddress(0))
            {
                delete newProbe;
                
                prober->setTimeout(usedTimeout * 4);
                try
                {
                    newProbe = this->probe(probeDst, probeTTL);
                }
                catch(SocketException e)
                {
                    this->stop();
                    return;
                }
                rplyAddress = newProbe->getRplyAddress();
            }
            
            // Restores default timeout
            prober->setTimeout(usedTimeout);
        }
    
        interfaces.push_back(newProbe->getRplyAddress());
        delete newProbe;

        probeTTL++;
    }
    
    if(probeTTL == ParisTracerouteTask::MAX_HOPS || interfaces.size() == 0)
    {
        this->abort();
        return;
    }
    
    /*
     * Just to be sure, some backward probing is performed in case the TTL estimation was too 
     * pessimistic (i.e. too high) due to networking issues.
     */
    
    while(probeTTL > 1)
    {
        ProbeRecord *newProbe = NULL;
        try
        {
            newProbe = this->probe(probeDst, probeTTL - 1);
        }
        catch(SocketException e)
        {
            this->stop();
            return;
        }
    
        // Decreases TTL and pops out last interface if an ECHO reply is obtained
        bool decreased = false;
        if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY) 
        {
            probeTTL--;
            interfaces.pop_back();
            decreased = true;
        }
        
        delete newProbe;
        
        if(!decreased)
            break;
    }
    
    // Something that should not happen still happened; we stop here
    if(probeTTL < 1)
    {
        this->abort();
        return;
    }
    
    /*
     * At this point, we got the new route. We replace the old route.
     */
    
    unsigned short sizeRoute = (unsigned short) interfaces.size();
    RouteInterface *route = new RouteInterface[sizeRoute];
    RouteInterface *oldRoute = subnet->getRoute();
    subnet->setRoute(NULL);
    
    if(oldRoute != NULL)
        delete[] oldRoute;
    
    unsigned short index = 0;
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        route[index].ip = (*i);
        if((*i) != InetAddress(0))
            route[index].state = RouteInterface::VIA_TRACEROUTE;
        else
            route[index].state = RouteInterface::MISSING;
        index++;
    }
    
    subnet->setRouteTarget(probeDst);
    subnet->setRouteSize(sizeRoute);
    subnet->setRoute(route);
    
    unsigned short status = subnet->getStatus();
    
    // Stops here if subnet was considered to be a SHADOW one
    if(status == SubnetSite::SHADOW_SUBNET)
    {
        subnet->adaptTTLs(probeTTL);
        subnet->completeRefinedData(); // To update shortest/greatest TTL
        
        if(displayFinalRoute)
        {
            stringstream routeLog;
            
            // Adding a line break after probe logs to separate from the complete route
            if(debugMode)
            {
                routeLog << "\n";
            }
            
            routeLog << "Got the route to " << subnet->getInferredNetworkAddressString() << ":\n";
            for(unsigned short i = 0; i < sizeRoute; i++)
            {
                if(route[i].state == RouteInterface::MISSING)
                    routeLog << "Missing\n";
                else
                    routeLog << route[i].ip << "\n";
            }

            this->log += routeLog.str();
        }
        else
        {
            this->log += "Got the route to " + subnet->getInferredNetworkAddressString() + ".\n";
        }
        
        // Prints out the final log (with debug messages, if any)
        TreeNETEnvironment::consoleMessagesMutex.lock();
        ostream *out = env->getOutputStream();
        (*out) << this->log << endl;
        TreeNETEnvironment::consoleMessagesMutex.unlock();
        
        return;
    }

    /*
     * Exceptional case where probeTTL equals 1 (probes with probeTTL-1 are irrelevant), 
     * which is necessarily a Contra-Pivot (TTL = 0 means we are on the subnet, which is not the 
     * intended use of TreeNET). Some repositioning is needed.
     */
    
    if(probeTTL == 1)
    {
        InetAddress newPivot(0);
        
        if(debugMode)
        {
            this->log += "\nRepositioning the subnet because TTL to Pivot node is equal to 1...\n";
        }

        list<SubnetSiteNode*> *nodes = subnet->getSubnetIPList();
        for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            if(cur->ip == probeDst)
            {
                cur->TTL = probeTTL;
                continue;
            }
        
            // First probe at probeTTL
            ProbeRecord *validationProbe = NULL;
            try
            {
                validationProbe = this->probe(cur->ip, probeTTL);
            }
            catch(SocketException e)
            {
                this->stop();
                return;
            }

            // We get an echo reply at probeTTL, so it must be another Contra-Pivot
            if(validationProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                cur->TTL = probeTTL;
            }
            // Otherwise it should be a Pivot
            else
            {
                cur->TTL = probeTTL + 1;
                newPivot = cur->ip;
            }
            
            delete validationProbe;
        }
        
        // Fixing last step of the route
        ProbeRecord *lastProbe = NULL;
        try
        {
            lastProbe = this->probe(newPivot, probeTTL);
        }
        catch(SocketException e)
        {
            this->stop();
            return;
        }
        route[sizeRoute - 1].ip = lastProbe->getRplyAddress();
        route[sizeRoute - 1].state = RouteInterface::VIA_TRACEROUTE;
        delete lastProbe;
        
        subnet->completeRefinedData();
        
        // Prints out result
        std::string subnetStr = subnet->getInferredNetworkAddressString();
        stringstream routeLog;
        if(displayFinalRoute)
        {
            if(debugMode)
                routeLog << "\n"; // For airy display
            routeLog << "Got the route to " << subnetStr << ":\n";
            for(unsigned short i = 0; i < sizeRoute; i++)
            {
                if(route[i].ip == InetAddress(0))
                    routeLog << "Missing\n";
                else
                    routeLog << route[i].ip << "\n";
            }
            routeLog << "\nSubnet also repositioned due to different Pivot/Contra-Pivot nodes.\n";
        }
        else
        {
            if(debugMode)
                routeLog << "\n"; // For airy display
            routeLog << "Got the route to " << subnet->getInferredNetworkAddressString();
            routeLog << " (successfully repositioned).\n";
        }
        this->log += routeLog.str();
        
        // Prints out the final log (with debug messages, if any)
        TreeNETEnvironment::consoleMessagesMutex.lock();
        ostream *out = env->getOutputStream();
        (*out) << this->log << endl;
        TreeNETEnvironment::consoleMessagesMutex.unlock();
        
        return;
    }

    /*
     * SUBNET REPOSITIONING
     *
     * Depending on the vantage point, the Pivot/Contra-Pivot interfaces of a subnet can change. 
     * Therefore, it is necessary to perform additional probes to ensure the soundness of the 
     * subnet by updating the TTL of its listed interfaces. The method consists in first verifying 
     * that a Contra-Pivot from former measurements is still a Contra-Pivot, and if it is not, 
     * additionnal probes to find the new Contra-Pivot interfaces (which are necessarily among the 
     * listed responsive interfaces) are used.
     */
    
    unsigned short nbCp = subnet->countContrapivotAddresses();
    bool needsRepositioning = false;
    
    // Single contra-pivot to check
    if(nbCp == 1)
    {
        InetAddress contrapivotIP = subnet->getContrapivot();
        for(unsigned short attempts = 0; attempts < 3; attempts++)
        {
            ProbeRecord *cpCheck = NULL;
            try
            {
                cpCheck = this->probe(contrapivotIP, probeTTL - 1);
            }
            catch(SocketException e)
            {
                this->stop();
                return;
            }
            unsigned char probeResult = cpCheck->getRplyICMPtype();
            delete cpCheck;
        
            if(probeResult == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                needsRepositioning = true;
                break;
            }
            else if(probeResult == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                break;
            }
        }
    }
    else
    {
        list<InetAddress> candiCp = subnet->getContrapivotAddresses();
        for(list<InetAddress>::iterator it = candiCp.begin(); it != candiCp.end(); ++it)
        {
            InetAddress contrapivotIP = (*it);
            
            ProbeRecord *cpCheck = NULL;
            try
            {
                cpCheck = this->probe(contrapivotIP, probeTTL - 1);
            }
            catch(SocketException e)
            {
                this->stop();
                return;
            }
            unsigned char probeResult = cpCheck->getRplyICMPtype();
            delete cpCheck;
        
            if(probeResult == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                needsRepositioning = true;
                break;
            }
            else if(probeResult == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                break;
            }
        } 
    }
    
    bool successfulRepositioning = false; // To tell later the user if it turned out well
    if(needsRepositioning)
    {
        if(debugMode)
        {
            this->log += "\nSubnet needs repositioning because former contra-pivot IP(s) ";
            this->log += "do not reply at expected TTL.\n";
        }
    
        // Looking for new Contra-Pivot candidates
        list<SubnetSiteNode*> *nodes = subnet->getSubnetIPList();
        unsigned short initialSTTL = subnet->getShortestTTL();
        unsigned short cpCount = 0;
        for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
        {
            SubnetSiteNode* cur = (*i);
            
            ProbeRecord *testProbe = NULL;
            try
            {
                testProbe = this->probe(cur->ip, probeTTL - 1);
            }
            catch(SocketException e)
            {
                this->stop();
                return;
            }
            if(testProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                cur->TTL--; // adaptTTLs() will finish the work
                cpCount++;
            }
            else if(testProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                // A Contra-Pivot from previous measurements is now a Pivot; TTL is incremented
                if(cur->TTL == initialSTTL)
                {
                    cur->TTL++; // adaptTTLs() will finish the work
                }
            }
            delete testProbe;
        }
        
        // Found some Contra-Pivot nodes: OK, adapt TTLs and finish updating subnet
        if(cpCount > 0)
        {
            if(debugMode)
            {
                this->log += "\nFound new contra-pivot IP(s).\n";
            }
        
            subnet->adaptTTLs(probeTTL);
            subnet->completeRefinedData();
            
            successfulRepositioning = true;
        }
        // Otherwise, initial dest should be the Contra-Pivot
        else
        {
            if(debugMode)
            {
                this->log += "\nInitial destination might actually be a contra-pivot IP.\n";
            }
            
            unsigned short pivotCount = 0;
            InetAddress finalRouteStep(0); // Set if we find a pivot
            for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
            {
                SubnetSiteNode* cur = (*i);
                
                if(cur->ip == probeDst)
                {
                    cur->TTL = probeTTL;
                    continue;
                }
            
                ProbeRecord *testProbe = NULL;
                try
                {
                    testProbe = this->probe(cur->ip, probeTTL);
                }
                catch(SocketException e)
                {
                    this->stop();
                    return;
                }
                
                // Another contra-Pivot
                if(testProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    cur->TTL = probeTTL;
                }
                // Necessarily a Pivot
                else
                {
                    pivotCount++;
                    if(finalRouteStep == InetAddress(0))
                        finalRouteStep = testProbe->getRplyAddress();
                    cur->TTL = probeTTL + 1;
                }
                delete testProbe;
            }
            
            if(pivotCount > 0)
            {
                if(debugMode)
                {
                    this->log += "\nInitial destination was indeed a contra-pivot IP. Route was resized.\n";
                }
            
                // Resizing the route and completing it
                sizeRoute++;
                RouteInterface *finalRoute = new RouteInterface[sizeRoute];
                for(unsigned short i = 0; i < sizeRoute - 1; i++)
                {
                    finalRoute[i].ip = route[i].ip;
                    finalRoute[i].state = route[i].state;
                }
                finalRoute[sizeRoute - 1].ip = finalRouteStep;
                finalRoute[sizeRoute - 1].state = RouteInterface::VIA_TRACEROUTE;
                
                delete[] route;
                
                route = finalRoute;
                
                subnet->setRouteSize(sizeRoute);
                subnet->setRoute(finalRoute);
                
                successfulRepositioning = true;
            }
            else
            {
                if(debugMode)
                {
                    this->log += "\nUnable to find contra-pivot IP(s).\n";
                }
            }
        }
        
        /*
         * N.B.: last case might not give the exact TTLs for potential TTLs, which will cause a 
         * small bias. However, this would require more probing work, and this case is not 
         * supposed to occur on a regular basis.
         */
        
        subnet->completeRefinedData();
    }
    else
    {
        subnet->adaptTTLs(probeTTL);
        subnet->completeRefinedData(); // To update shortest/greatest TTL
    }
    
    // Computes the log
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    stringstream routeLog;
    if(displayFinalRoute)
    {
        if(debugMode)
            routeLog << "\n"; // For airy display
        routeLog << "Got the route to " << subnetStr << ":\n";
        for(unsigned short i = 0; i < sizeRoute; i++)
        {
            if(route[i].ip == InetAddress(0))
                routeLog << "Missing\n";
            else
                routeLog << route[i].ip << "\n";
        }
        if(needsRepositioning)
        {
            if(successfulRepositioning)
                routeLog << "\nSubnet also repositioned due to different pivot/contra-pivot IP(s).\n";
            else
                routeLog << "\nAttempted repositioning but did not find any contra-pivot IP.\n";
        }
    }
    else
    {
        if(debugMode)
            routeLog << "\n"; // For airy display
        routeLog << "Got the route to " << subnet->getInferredNetworkAddressString();
        if(needsRepositioning)
        {
            if(successfulRepositioning)
                routeLog << " (successfully repositioned)";
            else
                routeLog << " (failed repositioning)";
        }
        routeLog << ".\n";
    }
    this->log += routeLog.str();
    
    // Displays the log, which can be a complete sequence of probe as well as a single line
    TreeNETEnvironment::consoleMessagesMutex.lock();
    ostream *out = env->getOutputStream();
    (*out) << this->log << endl;
    TreeNETEnvironment::consoleMessagesMutex.unlock();
}
