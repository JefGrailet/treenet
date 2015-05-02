/*
 * IPIDResolverUnit.h
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes a single IP and retrieves the IP identifier found in 
 * the ICMP reply. This identifier is stored, along a token, in an InetAddress object which 
 * corresponds to the probed IP.
 *
 * As its name suggests, IPIDResolverUnit is used by AliasResolver for its probing. The 
 * AliasResolver object itself must be given in the constructor to obtain a probe token.
 */

#ifndef IPIDRESOLVERUNIT_H_
#define IPIDRESOLVERUNIT_H_

#include "../common/thread/Runnable.h"
#include "../common/inet/InetAddress.h"
#include "../common/date/TimeVal.h"
#include "../common/thread/Mutex.h"
#include "../prober/DirectProber.h"
#include "../prober/icmp/DirectICMPProber.h"
#include "../prober/exception/SocketException.h"
#include "../prober/structure/ProbeRecord.h"
#include "AliasResolver.h"

class IPIDResolverUnit : public Runnable
{
public:

    // TTL value used in all packets
    static const unsigned char PROBE_TTL = 64;

    // Mutual exclusion object used when taking a probe token
    static Mutex resolverMutex;
    
    // Constructor
    IPIDResolverUnit(AliasResolver *parent,
                     InetAddress *IPToProbe,
                     InetAddress &localIPAddress,
                     string &attentionMessage,
                     bool useFixedFlowID = true,
                     const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD,
                     const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
                     unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                     unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                     unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                     unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor and run method
    ~IPIDResolverUnit();
    void run();
    
private:
    
    // Private fields
    AliasResolver *parent;
    InetAddress *IPToProbe;
    
    DirectProber *prober;
    InetAddress &localIPAddress;
    bool useFixedFlowID;
    
    // Probing methods
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);

};

#endif /* IPIDRESOLVERUNIT_H_ */
