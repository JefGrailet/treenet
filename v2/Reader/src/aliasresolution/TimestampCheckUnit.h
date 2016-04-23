/*
 * TimestampCheckUnit.h
 *
 *  Created on: April 13, 2016
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes a single IP with an ICMP timestamp request (code = 13) 
 * and expects a timestamp reply (code = 14). This mechanism of ICMP is not always implemented in 
 * routers, hence the interesting of checking an IP supports it to fingerprint it for the next 
 * steps of alias resolution.
 */

#ifndef TIMESTAMPCHECKUNIT_H_
#define TIMESTAMPCHECKUNIT_H_

#include "../utils/TreeNETEnvironment.h"
#include "../common/thread/Runnable.h"
#include "../common/inet/InetAddress.h"
#include "../common/date/TimeVal.h"
#include "../prober/DirectProber.h"
#include "../prober/icmp/DirectICMPProber.h"
#include "../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../prober/exception/SocketException.h"
#include "../prober/structure/ProbeRecord.h"

class TimestampCheckUnit : public Runnable
{
public:

    // TTL value used in all packets
    static const unsigned char PROBE_TTL = 64;
    
    // Constructor
    TimestampCheckUnit(TreeNETEnvironment *env, 
                       InetAddress IPToProbe, 
                       unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                       unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                       unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                       unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor and run method
    ~TimestampCheckUnit();
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

#endif /* TIMESTAMPCHECKUNIT_H_ */
