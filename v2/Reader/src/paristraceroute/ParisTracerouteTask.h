/*
 * ParisTracerouteTask.h
 *
 *  Created on: Mar 16, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, is given a subnet and is responsible for computing a route
 * to it (i.e. a list of interfaces, each of them assumed to be an interface of the router at a 
 * given hop count on the way to a same destination).
 *
 * The way the route is obtained is identical to Paris traceroute (which aims at keeping the same 
 * flow for all packets targetting the same destination), hence the name of the class. However, 
 * the technique itself is not implemented here, but was already implemented in ExploreNET's 
 * probing tools.
 *
 * The implementation as a class inheriting Runnable allows to parallelize the obtention of each 
 * route to speed up TreeNET through multi-threading.
 *
 * Note (11/01/2016): ParisTracerouteTask in TreeNET Reader also includes additionnal code to 
 * estimate the minimum TTL to reach the destination subnet using up to 5 Pivot interfaces, and 
 * also double checks if the Pivot/Contra-Pivot labelling as provided still holds. If it finds out 
 * the former Contra-Pivot node is now a simple Pivot (because changing the VP also changed the 
 * way the subnet was positioned with respect to VP), the TTL of each responsive IP is double 
 * checked and unresponsive IPs are dropped.
 */

#ifndef PARISTRACEROUTETASK_H_
#define PARISTRACEROUTETASK_H_

#include <list>
using std::list;

#include "../utils/TreeNETEnvironment.h"
#include "../common/thread/Runnable.h"
#include "../common/inet/InetAddress.h"
#include "../common/date/TimeVal.h"
#include "../common/thread/Mutex.h"
#include "../prober/DirectProber.h"
#include "../prober/icmp/DirectICMPProber.h"
#include "../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../prober/exception/SocketException.h"
#include "../prober/structure/ProbeRecord.h"
#include "../structure/SubnetSite.h"

class ParisTracerouteTask : public Runnable
{
public:

    // Maximum distance (in hops) for a pivot
    static const unsigned short MAX_HOPS = 40;
    
    // Maximum amount of pivot addresses being tested to re-compute the route
    static const unsigned short MAX_PIVOT_CANDIDATES = 5;

    // Mutual exclusion object used when writing on the output
    static Mutex ostreamMutex;
    
    // Constructor
    ParisTracerouteTask(TreeNETEnvironment *env, 
                        list<SubnetSite*> *toDelete,
                        SubnetSite *subnet, 
                        unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                        unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                        unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                        unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor, run method and print out method
    ~ParisTracerouteTask();
    void run();
    
private:
    
    // Pointer to the environment variable and to the subnet to modify
    TreeNETEnvironment *env;
    
    // List of subnets which will be deleted later, because we cannot recompute the route
    list<SubnetSite*> *toDelete;
    
    // Subnet for which route is being re-computed
    SubnetSite *subnet;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);

    // "Abort" method (when we cannot recompute the route)
    void abort();
};

#endif /* PARISTRACEROUTETASK_H_ */
