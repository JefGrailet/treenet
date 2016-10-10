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
#include "../../../structure/RouteInterface.h"

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
    
    // Verbosity/debug stuff
    displayFinalRoute = false; // Default
    debugMode = false; // Default
    switch(env->getDisplayMode())
    {
        case TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE:
        case TreeNETEnvironment::DISPLAY_MODE_VERBOSE:
            displayFinalRoute = true;
            break;
        case TreeNETEnvironment::DISPLAY_MODE_DEBUG:
            displayFinalRoute = true;
            debugMode = true;
            break;
        default:
            break;
    }
    
    // Start of the new log with first probing details (debug mode only)
    if(debugMode)
    {
        this->log += "Computing route to " + subnet->getInferredNetworkAddressString() + "...\n";
        this->log += prober->getAndClearLog();
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
        record = prober->doubleProbe(localIP, dst, TTL, true);
    else
        record = prober->singleProbe(localIP, dst, TTL, true);
    
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
    RouteInterface *route = new RouteInterface[sizeRoute];
    
    while(probeTTL > 0)
    {
        try
        {
            ProbeRecord *record = this->probe(probeDst, probeTTL);
            
            InetAddress rplyAddress = record->getRplyAddress();
            if(rplyAddress == InetAddress("0.0.0.0"))
            {
                delete record;
                
                // Debug message
                if(debugMode)
                {
                    this->log += "Retrying at this TTL with twice the initial timeout...\n";
                }
                
                // New probe with twice the timeout period
                prober->setTimeout(usedTimeout * 2);
                record = this->probe(probeDst, probeTTL);
                rplyAddress = record->getRplyAddress();
                
                // If still nothing different than 0.0.0.0, last try with 4 times the timeout
                if(rplyAddress == InetAddress("0.0.0.0"))
                {
                    delete record;
                    
                    // Debug message
                    if(debugMode)
                    {
                        this->log += "Retrying at this TTL with 4 times the initial timeout...\n";
                    }
                    
                    prober->setTimeout(usedTimeout * 4);
                    record = this->probe(probeDst, probeTTL);
                    rplyAddress = record->getRplyAddress();
                }
                
                // Restores default timeout
                prober->setTimeout(usedTimeout);
            }
            
            route[probeTTL - 1].update(rplyAddress);
            
            delete record;
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
    
    subnet->setRouteSize(sizeRoute);
    subnet->setRoute(route);
    
    // Appends the log with a single line (laconic mode) or the route that was obtained
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
                routeLog << "Missing" << "\n";
            else
                routeLog << route[i].ip << "\n";
        }

        this->log += routeLog.str();
    }
    else
    {
        this->log += "Got the route to " + subnet->getInferredNetworkAddressString() + ".";
    }
    
    // Displays the log, which can be a complete sequence of probe as well as a single line
    TreeNETEnvironment::consoleMessagesMutex.lock();
    ostream *out = env->getOutputStream();
    (*out) << this->log << endl;
    TreeNETEnvironment::consoleMessagesMutex.unlock();
}
