/*
 * IPIDUnit.h
 *
 *  Created on: Feb 27, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, probes a single IP and retrieves the IP identifier found in 
 * the ICMP reply. The results are stored within IPIDUnit, but can later be retrieved by the 
 * calling code with a IPIDTuple object created by it.
 *
 * As its name suggests, IPIDUnit is used by IPIDCollector for its probing, which itself has a 
 * AliasHintCollector object as parent. This same object must be provided in the constructor to 
 * be able to obtain a probe token.
 */

#ifndef IPIDUNIT_H_
#define IPIDUNIT_H_

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
#include "AliasHintCollector.h"
#include "IPIDTuple.h" // Provides <ctime> as well

class IPIDUnit : public Runnable
{
public:

    // TTL value used in all packets
    static const unsigned char PROBE_TTL = 64;

    // Mutual exclusion object used when taking a probe token
    static Mutex collectorMutex;
    
    // Constructor
    IPIDUnit(TreeNETEnvironment *env, 
             AliasHintCollector *grandParent, 
             InetAddress IPToProbe, 
             unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
             unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
             unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
             unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw (SocketException);
    
    // Destructor and run method
    ~IPIDUnit();
    void run();
    
    // Methods to check results and get a IPIDTuple object after completion of run()
    bool hasExploitableResults();
    IPIDTuple *getTuple();
    
private:

    // Pointer to the environment object (=> probing parameters)
    TreeNETEnvironment *env;
    
    // Private fields
    AliasHintCollector *grandParent;
    InetAddress IPToProbe;
    
    // Private fields to maintain results
    unsigned short probeToken;
    unsigned long int IPID;
    timeval timeValue;
    bool echo;
    unsigned char replyTTL;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    TimeVal baseTimeout;

};

#endif /* IPIDRUNIT_H_ */
