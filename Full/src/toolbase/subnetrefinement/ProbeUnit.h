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

#include "../../common/thread/Runnable.h"
#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"
#include "ProbesDispatcher.h"

class ProbeUnit : public Runnable
{
public:

    // Mutual exclusion object used when reading/editing ProbesDispatcher
    static Mutex dispatcherMutex;
    
    // Constructor
    ProbeUnit(ProbesDispatcher *parent,
              std::list<InetAddress> IPsToProbe,
              unsigned char requiredTTL,
              unsigned char alternativeTTL,
              InetAddress &localIPAddress,
              string &attentionMessage,
              bool useFixedFlowID = true,
              const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD,
              const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
              unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
              unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
              unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
              unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE) throw(SocketException);
    
    // Destructor, run method and print out method
    ~ProbeUnit();
    void run();
    
private:
    
    // Private fields
    ProbesDispatcher *parent;
    std::list<InetAddress> IPsToProbe;
    unsigned char requiredTTL, alternativeTTL;
    
    DirectProber *prober;
    InetAddress &localIPAddress;
    bool useFixedFlowID;
    
    // Probing methods
    ProbeRecord *probe(const InetAddress &dst, unsigned char TTL);
    ProbeRecord *doubleProbe(const InetAddress &dst, unsigned char TTL);

};

#endif /* PROBEUNIT_H_ */
