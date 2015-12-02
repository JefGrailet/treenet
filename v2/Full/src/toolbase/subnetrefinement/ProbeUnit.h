/*
 * ProbeUnit.h
 *
 *  Created on: Oct 30, 2014
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes successively each IP it is given (via a list). It is
 * the class used for the threads of ProbesDispatcher class.
 */

#ifndef PROBEUNIT_H_
#define PROBEUNIT_H_

#include <list>
using std::list;

#include "../TreeNETEnvironment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"
#include "ProbesDispatcher.h"

class ProbeUnit : public Runnable
{
public:

    // Mutual exclusion object used when reading/editing ProbesDispatcher
    static Mutex dispatcherMutex;
    
    // Constructor
    ProbeUnit(TreeNETEnvironment *env, 
              ProbesDispatcher *parent, 
              std::list<InetAddress> IPsToProbe, 
              unsigned char requiredTTL, 
              unsigned char alternativeTTL, 
              unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
              unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
              unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
              unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ) throw(SocketException);
    
    // Destructor, run method
    ~ProbeUnit();
    void run();
    
private:
    
    // Pointer to environment (with prober parameters)
    TreeNETEnvironment *env;
    
    // Private fields
    ProbesDispatcher *parent;
    std::list<InetAddress> IPsToProbe;
    unsigned char requiredTTL, alternativeTTL;

    // Prober object and probing methods
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);

};

#endif /* PROBEUNIT_H_ */
