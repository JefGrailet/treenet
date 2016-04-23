/*
 * UDPUnreachablePortUnit.h
 *
 *  Created on: April 15, 2016
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes a single IP with a single ICMP packet wrapped in UDP, 
 * targetting an unlikely high destination port number. The subsequent ICMP Port Unreachable 
 * message (if received) should contain a source address belonging to the router which bears the 
 * initial destination. This address is not always the same as the initial destination, hence the 
 * relevance of the whole mechanism for alias resolution.
 */

#ifndef UDPUNREACHABLEPORTUNIT_H_
#define UDPUNREACHABLEPORTUNIT_H_

#include "../TreeNETEnvironment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"

class UDPUnreachablePortUnit : public Runnable
{
public:

    // TTL value used in all packets
    static const unsigned char PROBE_TTL = 64;
    
    // Constructor
    UDPUnreachablePortUnit(TreeNETEnvironment *env, 
                           InetAddress IPToProbe, 
                           unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                           unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                           unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                           unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor and run method
    ~UDPUnreachablePortUnit();
    void run();
    
private:

    // Pointer to the environment object (=> probing parameters + access to IP dictionnary)
    TreeNETEnvironment *env;
    
    // IP to check
    InetAddress IPToProbe;

    // Probing stuff
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    TimeVal baseTimeout;

};

#endif /* UDPUNREACHABLEPORTUNIT_H_ */
