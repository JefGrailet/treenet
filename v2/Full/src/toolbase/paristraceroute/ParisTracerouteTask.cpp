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
                                         SubnetSite *ss, 
                                         unsigned short lbii, 
                                         unsigned short ubii, 
                                         unsigned short lbis, 
                                         unsigned short ubis) throw (SocketException):
env(e), 
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

void ParisTracerouteTask::run()
{
    InetAddress probeDst(subnet->getRefinementPivot());
    unsigned char probeTTL = subnet->getRefinementShortestTTL(); // TTL pivot - 1

    // probeTTL or probeDst could not be properly found: quits
    if(probeTTL == 0 || probeDst == InetAddress(0))
    {
        return;
    }
    
    // Changes timeout if necessary for this IP
    IPLookUpTable *table = env->getIPTable();
    IPTableEntry *probeDstEntry = table->lookUp(probeDst);
    TimeVal initialTimeout, preferredTimeout, usedTimeout;
    initialTimeout = prober->getTimeout();
    if(probeDstEntry != NULL)
        preferredTimeout = probeDstEntry->getPreferredTimeout();
    
    bool timeoutChanged = false;
    if(preferredTimeout > initialTimeout)
    {
        timeoutChanged = true;
        prober->setTimeout(preferredTimeout);
        usedTimeout = preferredTimeout;
    }
    else
        usedTimeout = initialTimeout;
    
    // Route array
    unsigned short sizeRoute = (unsigned short) probeTTL;
    InetAddress *route = new InetAddress[sizeRoute];
    
    while(probeTTL > 0)
    {
        try
        {
            ProbeRecord *probe = this->probe(probeDst, probeTTL);
            
            InetAddress rplyAddress = probe->getRplyAddress();
            if(rplyAddress == InetAddress("0.0.0.0"))
            {
                delete probe;
                
                // New probe with twice the timeout period
                prober->setTimeout(usedTimeout * 2);
                probe = this->probe(probeDst, probeTTL);
                rplyAddress = probe->getRplyAddress();
                
                // If still nothing different than 0.0.0.0, last try with 4 times the timeout
                if(rplyAddress == InetAddress("0.0.0.0"))
                {
                    delete probe;
                    
                    prober->setTimeout(usedTimeout * 4);
                    probe = this->probe(probeDst, probeTTL);
                    rplyAddress = probe->getRplyAddress();
                }
                
                // Restores default timeout
                prober->setTimeout(usedTimeout);
            }
            
            route[probeTTL - 1] = rplyAddress;
            
            delete probe;
        }
        catch(SocketSendException e)
        {
            probeTTL++;
        }
        catch(SocketReceiveException e)
        {
            probeTTL++;
        }
        probeTTL--;
    }
    
    // Restores previous timeout value if it was changed
    if(timeoutChanged)
    {
        prober->setTimeout(initialTimeout);
    }
    
    subnet->setRefinementRouteSize(sizeRoute);
    subnet->setRefinementRoute(route);
    
    // Prints out the obtained route
    ostreamMutex.lock();
    
    ostream *out = env->getOutputStream();
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    (*out) << "Route to " << subnetStr << ":" << endl;
    for(unsigned short i = 0; i < sizeRoute; i++)
        (*out) << route[i] << endl;
    (*out) << endl;
    
    ostreamMutex.unlock();
}
