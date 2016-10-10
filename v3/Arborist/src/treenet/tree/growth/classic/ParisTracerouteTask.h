/*
 * ParisTracerouteTask.h
 *
 *  Created on: Mar 16, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, gets a subnet and is responsible for computing a route to it 
 * (i.e. a list of interfaces, each of them assumed to be an interface of the router at a given 
 * hop count on the way to a same destination).
 *
 * The way the route is obtained is identical to Paris traceroute (which aims at keeping the same 
 * flow for all packets targetting the same destination), hence the name of the class. However, 
 * the technique itself is not implemented here, as it is already implemented in ExploreNET's 
 * probing tools via the fixed flow boolean parameter.
 *
 * The implementation as a class inheriting Runnable allows to parallelize the obtention of each 
 * route to speed up TreeNET through multi-threading.
 *
 * History of modifications:
 * -Augustus 2016: modified the class such that it can build up a log which content depends on the 
 *  verbosity mode. In debug mode, it concatenates the different logs for each successive probe 
 *  and ends with the route it obtains in the end. In slightly verbose, it only displays the final 
 *  route. By default, it only shows a message "Got the route to [subnet].".
 * -Sept 8, 2016: adapted the class to take account of the new RouteInterface class (structure/). 
 *  also moved to a different folder to keep a coherent file architecture.
 */

#ifndef PARISTRACEROUTETASK_H_
#define PARISTRACEROUTETASK_H_

#include "../../../TreeNETEnvironment.h"
#include "../../../../common/thread/Runnable.h"
#include "../../../../common/thread/Mutex.h"
#include "../../../../prober/DirectProber.h"
#include "../../../../prober/icmp/DirectICMPProber.h"
#include "../../../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../../../prober/exception/SocketException.h"
#include "../../../../prober/structure/ProbeRecord.h"
#include "../../../structure/SubnetSite.h"

class ParisTracerouteTask : public Runnable
{
public:

    // Constructor
    ParisTracerouteTask(TreeNETEnvironment *env, 
                        SubnetSite *subnet, 
                        unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                        unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                        unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                        unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor, run method and print out method
    ~ParisTracerouteTask();
    void run();
    
private:

    // Pointer to the environment variable
    TreeNETEnvironment *env;
    
    // Pointer to the subnet for which the route is being computed
    SubnetSite *subnet;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    
    // Verbosity/debug stuff
    bool displayFinalRoute, debugMode;
    string log;
};

#endif /* PARISTRACEROUTETASK_H_ */
