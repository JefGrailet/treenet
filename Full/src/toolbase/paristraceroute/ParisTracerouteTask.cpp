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

ParisTracerouteTask::ParisTracerouteTask(ostream *o,
                                         SubnetSite *ss,
                                         InetAddress &lIPa,
                                         string &msg,
                                         const TimeVal &tp,
                                         const TimeVal &prpp,
                                         unsigned short lbii,
                                         unsigned short ubii,
                                         unsigned short lbis,
                                         unsigned short ubis) throw (SocketException):
out(o),
subnet(ss),
localIPAddress(lIPa)
{
    try
    {
        prober = new DirectICMPProber(msg, tp, prpp, lbii, ubii, lbis, ubis);
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
    ProbeRecord *record = NULL;
    record = prober->doubleProbe(this->localIPAddress, dst, TTL, true, false, 0);

    return record;
}

void ParisTracerouteTask::run()
{
    InetAddress probeDst = subnet->getRefinementPivot();
    unsigned char probeTTL = subnet->getRefinementShortestTTL(); // TTL pivot - 1

    // probeTTL or probeDst could not be properly found: quits
    if(probeTTL == 0 || probeDst == InetAddress(0))
    {
        return;
    }
    
    unsigned short sizeRoute = (unsigned short) probeTTL;
    InetAddress *route = new InetAddress[sizeRoute];
    
    while(probeTTL > 0)
    {
        try
        {
            ProbeRecord *probe = this->probe(probeDst, probeTTL);
            
            route[probeTTL - 1] = probe->getRplyAddress();
            
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
    
    subnet->setRefinementRouteSize(sizeRoute);
    subnet->setRefinementRoute(route);
    
    // Prints out the obtained route
    ostreamMutex.lock();
    
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    (*out) << "Route to " << subnetStr << ":" << endl;
    for(unsigned short i = 0; i < sizeRoute; i++)
        (*out) << route[i] << endl;
    (*out) << endl;
    
    ostreamMutex.unlock();
}
