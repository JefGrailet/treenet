/*
 * AnonymousCheckUnit.h
 *
 *  Created on: Feb 16, 2017
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes a pivot IP within a subnet obtained from a list with 
 * the amount of hops corresponding to a missing hop (which is either truly anonymous, either 
 * "temporarily" anonymous because of security measures) in order to confirm if this hop is really 
 * anonymous or if this could not be obtained at first because of rate-limiting.
 *
 * As there can be several missing hops in a route, this class will use the right TTLs for each 
 * and therefore probe the same pivot IP several times. When a reply is obtained, it calls back 
 * the parent AnonymousChecker object to perform the route fix (plus propagation in similar 
 * routes).
 *
 * Overall architecture is based on the pre-scanning module.
 */

#ifndef ANONYMOUSCHECKUNIT_H_
#define ANONYMOUSCHECKUNIT_H_

#include <list>
using std::list;

#include "AnonymousChecker.h"
#include "../../../../common/thread/Runnable.h"
#include "../../../../prober/DirectProber.h"
#include "../../../../prober/icmp/DirectICMPProber.h"
#include "../../../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../../../prober/exception/SocketException.h"
#include "../../../../prober/structure/ProbeRecord.h"

class AnonymousCheckUnit : public Runnable
{
public:

    // Mutual exclusion object used when accessing AnonymousChecker
    static Mutex parentMutex;
    
    // Constructor
    AnonymousCheckUnit(TreeNETEnvironment *env, 
                       AnonymousChecker *parent, 
                       list<SubnetSite*> toReprobe, 
                       unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                       unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
                       unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                       unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ) throw(SocketException);
    
    // Destructor, run method
    ~AnonymousCheckUnit();
    void run();
    
private:
    
    // Pointer to environment (with prober parameters)
    TreeNETEnvironment *env;
    
    // Private fields
    AnonymousChecker *parent;
    list<SubnetSite*> toReprobe;

    // Prober object and probing methods
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    
    // "Stop" method (when resources are lacking)
    void stop();

};

#endif /* ANONYMOUSCHECKUNIT_H_ */
