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
                                         list<SubnetSite*> *td,
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
toDelete(td),
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

void ParisTracerouteTask::abort()
{
    toDelete->push_back(subnet);
    
    ostreamMutex.lock();
    
    (*out) << "Unable to recompute the route to ";
    (*out) << subnet->getInferredNetworkAddressString();
    (*out) << ". This subnet will be ignored." << endl << endl;
    
    ostreamMutex.unlock();
}

void ParisTracerouteTask::run()
{
    InetAddress probeDst = subnet->getPivotAddress();
    
    // First check the IP is responsive
    try
    {
        ProbeRecord *firstProbe = this->probe(probeDst, (unsigned char) 64);
        
        if(firstProbe->getRplyICMPtype() != DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            delete firstProbe;
            this->abort();
            return;
        }
        delete firstProbe;
        
        // Code will proceed to compute the route from this point
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
        
            if(newProbe->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY) 
            {
                delete newProbe;
                break;
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
    
    if(probeTTL == ParisTracerouteTask::MAX_HOPS)
    {
        this->abort();
        return;
    }
    
    // New route
    unsigned short sizeRoute = (unsigned short) interfaces.size();
    InetAddress *route = new InetAddress[sizeRoute];
    
    // Deleting old route
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
    
    subnet->adaptTTLs(probeTTL);
    subnet->setRouteSize(sizeRoute);
    subnet->setRoute(route);
    
    // Prints out the obtained route
    ostreamMutex.lock();
    
    std::string subnetStr = subnet->getInferredNetworkAddressString();
    (*out) << "Route to " << subnetStr << ":" << endl;
    for(unsigned short i = 0; i < sizeRoute; i++)
        (*out) << route[i] << endl;
    (*out) << endl;
    
    ostreamMutex.unlock();
}

