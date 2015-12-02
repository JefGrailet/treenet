/*
 * NetworkPrescanningUnit.h
 *
 *  Created on: Oct 8, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes successively each IP it is given (via a list), with a 
 * very large TTL (at this point, all we want is to known if the IP is responsive). It is the 
 * class used for the threads of NetworkPrescanner class.
 */

#ifndef NETWORKPRESCANNINGUNIT_H_
#define NETWORKPRESCANNINGUNIT_H_

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
#include "NetworkPrescanner.h"

class NetworkPrescanningUnit : public Runnable
{
public:

    static const unsigned char VIRTUALLY_INFINITE_TTL = (unsigned char) 255;

    // Mutual exclusion object used when accessing NetworkPrescanner
    static Mutex prescannerMutex;
    
    // Constructor
    NetworkPrescanningUnit(TreeNETEnvironment *env, 
                           NetworkPrescanner *parent, 
                           std::list<InetAddress> IPsToProbe, 
                           unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                           unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
                           unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                           unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ) throw(SocketException);
    
    // Destructor, run method
    ~NetworkPrescanningUnit();
    void run();
    
private:
    
    // Pointer to environment (with prober parameters)
    TreeNETEnvironment *env;
    
    // Private fields
    NetworkPrescanner *parent;
    std::list<InetAddress> IPsToProbe;

    // Prober object and probing methods (no TTL asked, since it is here virtually infinite)
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst);
    ProbeRecord *doubleProbe(const InetAddress &dst);

};

#endif /* NETWORKPRESCANNINGUNIT_H_ */
