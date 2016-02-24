/*
 * ParisTracerouteTask.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: grailet
 *
 * Implements the class defined in ParisTracerouteTask.h (see this file to learn further about the 
 * goals of such class).
 */

#include "ParisTracerouteTask.h"

Mutex ParisTracerouteTask::ostreamMutex(Mutex::ERROR_CHECKING_MUTEX);

ParisTracerouteTask::ParisTracerouteTask(TreeNETEnvironment *e, 
                                         list<SubnetSite*> *td, 
                                         SubnetSite *ss, 
                                         unsigned short lbii, 
                                         unsigned short ubii, 
                                         unsigned short lbis, 
                                         unsigned short ubis) throw (SocketException):
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
        throw e;
    }
}

ParisTracerouteTask::~ParisTracerouteTask()
{
    delete prober;
}

ProbeRecord *ParisTracerouteTask::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env->getLocalIPAddress();
    ProbeRecord *record = NULL;
    
    if(env->usingDoubleProbe())
        record = prober->doubleProbe(localIP, dst, TTL, true, false, 0);
    else
        record = prober->singleProbe(localIP, dst, TTL, true, false, 0);
    
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
    ostreamMutex.lock();
    
    (*out) << "Unable to recompute the route to ";
    (*out) << subnet->getInferredNetworkAddressString();
    (*out) << ". This subnet will be ignored." << endl << endl;
    
    ostreamMutex.unlock();
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
    
    // First check the IP is responsive
    try
    {
        ProbeRecord *firstProbe = this->probe(probeDst, ParisTracerouteTask::MAX_HOPS);
        while(firstProbe->getRplyICMPtype() != DirectProber::ICMP_TYPE_ECHO_REPLY && candidatesDst.size() > 0)
        {
            delete firstProbe;
            
            // Retries with another Pivot address
            probeDst = candidatesDst.front();
            candidatesDst.pop_front();
            firstProbe = this->probe(probeDst, ParisTracerouteTask::MAX_HOPS);
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
    }
    catch(SocketSendException e)
    {
        this->abort();
        return;
    }
    catch(SocketReceiveException e)
    {
        this->abort();
        return;
    }
    
    unsigned char probeTTL = 1;
    list<InetAddress> interfaces;
    while(probeTTL < ParisTracerouteTask::MAX_HOPS)
    {
        try
        {
            ProbeRecord *newProbe = this->probe(probeDst, probeTTL);
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
            if(rplyAddress == InetAddress("0.0.0.0"))
            {
                delete newProbe;
                
                // New probe with twice the timeout period
                prober->setTimeout(usedTimeout * 2);
                newProbe = this->probe(probeDst, probeTTL);
                rplyAddress = newProbe->getRplyAddress();
                
                // If still nothing different than 0.0.0.0, last try with 4 times the timeout
                if(rplyAddress == InetAddress("0.0.0.0"))
                {
                    delete newProbe;
                    
                    prober->setTimeout(usedTimeout * 4);
                    newProbe = this->probe(probeDst, probeTTL);
                    rplyAddress = newProbe->getRplyAddress();
                }
                
                // Restores default timeout
                prober->setTimeout(usedTimeout);
            }
        
            interfaces.push_back(newProbe->getRplyAddress());
            delete newProbe;
        }
        catch(SocketSendException e)
        {
            this->abort();
            return;
        }
        catch(SocketReceiveException e)
        {
            this->abort();
            return;
        }
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
        ProbeRecord *newProbe = this->probe(probeDst, probeTTL - 1);
    
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
    
    // New route replaces the old one
    unsigned short sizeRoute = (unsigned short) interfaces.size();
    InetAddress *route = new InetAddress[sizeRoute];
    InetAddress *oldRoute = subnet->getRoute();
    subnet->setRoute(NULL);
    
    if(oldRoute != NULL)
        delete[] oldRoute;
    
    unsigned short index = 0;
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        route[index] = (*i);
        index++;
    }
    
    subnet->setRouteSize(sizeRoute);
    subnet->setRoute(route);
    
    unsigned short status = subnet->getStatus();
    
    // Stops here if subnet was considered to be a SHADOW one
    if(status == SubnetSite::SHADOW_SUBNET)
    {
        subnet->adaptTTLs(probeTTL);
        subnet->completeRefinedData(); // To update shortest/greatest TTL
        
        ostream *out = env->getOutputStream();
        ostreamMutex.lock();
        
        std::string subnetStr = subnet->getInferredNetworkAddressString();
        (*out) << "Route to " << subnetStr << ":\n";
        for(unsigned short i = 0; i < sizeRoute; i++)
            (*out) << route[i] << "\n";
        (*out) << endl;
        
        ostreamMutex.unlock();
    }

    /*
     * Exceptional case where probeTTL equals 1 (probes with probeTTL-1 are irrelevant), 
     * which is necessarily a Contra-Pivot (TTL = 0 means we are on the subnet, which is not the 
     * intended use of TreeNET). Some repositioning is needed.
     */
    
    if(probeTTL == 1)
    {
        InetAddress newPivot(0);

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
            ProbeRecord *validationProbe = this->probe(cur->ip, probeTTL);

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
        ProbeRecord *lastProbe = this->probe(newPivot, probeTTL);
        route[sizeRoute - 1] = lastProbe->getRplyAddress();
        delete lastProbe;
        
        subnet->completeRefinedData();
        
        // Prints out results
        ostream *out = env->getOutputStream();
        ostreamMutex.lock();
        
        std::string subnetStr = subnet->getInferredNetworkAddressString();
        (*out) << "Route to " << subnetStr << ":\n";
        for(unsigned short i = 0; i < sizeRoute; i++)
            (*out) << route[i] << "\n";
        (*out) << "\nSubnet also repositioned due to different Pivot/Contra-Pivot nodes.\n";
        (*out) << endl;
        
        ostreamMutex.unlock();
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
            ProbeRecord *cpCheck = this->probe(contrapivotIP, probeTTL - 1);
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
            
            ProbeRecord *cpCheck = this->probe(contrapivotIP, probeTTL - 1);
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
    
    if(needsRepositioning)
    {
        // Looking for new Contra-Pivot candidates
        list<SubnetSiteNode*> *nodes = subnet->getSubnetIPList();
        unsigned short initialSTTL = subnet->getShortestTTL();
        unsigned short cpCount = 0;
        for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
        {
            SubnetSiteNode* cur = (*i);
            
            ProbeRecord *testProbe = this->probe(cur->ip, probeTTL - 1);
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
        
        // Found some Contra-Pivot: OK, adapt TTLs and finish updating subnet
        if(cpCount > 0)
        {
            subnet->adaptTTLs(probeTTL);
            subnet->completeRefinedData();   
        }
        // Otherwise, initial dest should be the Contra-Pivot
        else
        {
            for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
            {
                SubnetSiteNode* cur = (*i);
                
                if(cur->ip == probeDst)
                {
                    cur->TTL = probeTTL;
                    continue;
                }
            
                ProbeRecord *testProbe = this->probe(cur->ip, probeTTL);
                // Another contra-Pivot
                if(testProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    cur->TTL = probeTTL;
                }
                // Necessarily a Pivot
                else
                {
                    cur->TTL = probeTTL + 1;
                }
                delete testProbe;
            }
            
            // Fixing last step of the route
            ProbeRecord *lastProbe = this->probe(probeDst, probeTTL);
            route[sizeRoute - 1] = lastProbe->getRplyAddress();
            delete lastProbe;
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
    
    // Prints out the obtained route (+ additionnal message if subnet was repositioned)
    ostream *out = env->getOutputStream();
    ostreamMutex.lock();
    
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    (*out) << "Route to " << subnetStr << ":\n";
    for(unsigned short i = 0; i < sizeRoute; i++)
        (*out) << route[i] << "\n";
    if(needsRepositioning)
        (*out) << "\nSubnet also repositioned due to different Pivot/Contra-Pivot nodes.\n";
    (*out) << endl;
    
    ostreamMutex.unlock();
}
