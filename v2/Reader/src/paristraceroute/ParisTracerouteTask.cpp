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
        cout << "Case 0" << endl;
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
    
    // Pivot/Contrapivot validation: checks the contrapivot is still at probeTTL - 1
    InetAddress contrapivotIP = subnet->getContrapivot(); // Only uses the first one
    ProbeRecord *minusOne = NULL;
    if((unsigned short) probeTTL > 1)
        minusOne = this->probe(contrapivotIP, probeTTL - 1);
    
    bool repositioned = false;
    // At least minusOne must be non null (otherwise, probed Pivot can only be a Contra-Pivot)
    if(subnet->getStatus() == SubnetSite::SHADOW_SUBNET ||
       (minusOne != NULL && minusOne->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY))
    {
        subnet->adaptTTLs(probeTTL);
        if(minusOne != NULL)
            delete minusOne;
            
        subnet->completeRefinedData(); // To update shortest/greatest TTL
    }
    // Otherwise, one must double check the TTL of each IP and re-positions the subnet
    else
    {
        delete minusOne;
        
        /*
         * Additionnal values for when the IP with which TTL was evaluated is a Contra-Pivot 
         * from the new perspective (new vantage point).
         */
        
        bool wasContrapivot = false;
        InetAddress newPivot(0);
        
        list<SubnetSiteNode*> *nodes = subnet->getSubnetIPList();
        for(list<SubnetSiteNode*>::iterator i = nodes->begin(); i != nodes->end(); ++i)
        {
            if((*i)->ip == probeDst)
            {
                (*i)->TTL = probeTTL;
                continue;
            }
        
            // First probe at probeTTL
            ProbeRecord *validationProbe = this->probe((*i)->ip, probeTTL);
            bool success = false;
            
            // We get an echo reply at probeTTL, so we should also check probeTTL-1 to be sure
            if(validationProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                success = true; // Success whatever happens afterwards
                
                delete validationProbe;
                validationProbe = this->probe((*i)->ip, probeTTL - 1);
                
                if(validationProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                    (*i)->TTL = probeTTL - 1;
                else
                    (*i)->TTL = probeTTL;
                
                delete validationProbe;
            }
            // Reciprocally, we check at probeTTL+1 if there is no echo reply
            else
            {
                delete validationProbe;
                validationProbe = this->probe((*i)->ip, probeTTL + 1);
                
                // Initial target was a contra-pivot
                wasContrapivot = true;
                newPivot = (*i)->ip;
                
                if(validationProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    (*i)->TTL = probeTTL + 1;
                    success = true;
                }
                
                delete validationProbe;
            }
            
            // If we get nothing in the end, we delete this node
            if(!success)
            {
                delete (*i);
                nodes->erase(i--);
            }
        }
        
        // Fixing last step of the route if the initial probed IP was a Contra-Pivot
        if(wasContrapivot)
        {
            ProbeRecord *lastProbe = this->probe(newPivot, probeTTL);
            route[sizeRoute - 1] = lastProbe->getRplyAddress();
            delete lastProbe;
        }
        
        subnet->completeRefinedData();
        repositioned = true;
    }
    
    // Prints out the obtained route (+ additionnal message if subnet was repositioned)
    ostream *out = env->getOutputStream();
    ostreamMutex.lock();
    
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    (*out) << "Route to " << subnetStr << ":\n";
    for(unsigned short i = 0; i < sizeRoute; i++)
        (*out) << route[i] << "\n";
    if(repositioned)
        (*out) << "\nSubnet also repositioned due to different Pivot/Contra-Pivot nodes.\n";
    (*out) << endl;
    
    ostreamMutex.unlock();
}
