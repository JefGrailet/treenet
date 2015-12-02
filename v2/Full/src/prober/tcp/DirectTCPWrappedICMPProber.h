/*
 * DirectTCPWrappedICMPProber.h
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * Slightly edited in September 2015 by J.-F. Grailet to improve coding style.
 */

#ifndef DIRECTTCPWRAPPEDICMPPROBER_H_
#define DIRECTTCPWRAPPEDICMPPROBER_H_

#include "DirectTCPProber.h"
#include "../../common/inet/InetAddress.h"
#include "../exception/SocketSendException.h"
#include "../exception/SocketReceiveException.h"
#include "../exception/SocketException.h"
#include "../structure/ProbeRecord.h"
#include "../../common/date/TimeVal.h"

/**
 * Analyzes the probe results just before delivering and replaces all TCP RESET responses to ICMP 
 * ECHO REPLY.
 */

class DirectTCPWrappedICMPProber : public DirectTCPProber
{
public:
    DirectTCPWrappedICMPProber(string &attentionMessage, 
                               int tcpUdpRoundRobinSocketCount, 
                               const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD, 
                               const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD, 
                               unsigned short lowerBoundTCPsrcPort = DirectTCPProber::DEFAULT_LOWER_TCP_SRC_PORT, 
                               unsigned short upperBoundTCPsrcPort = DirectTCPProber::DEFAULT_UPPER_TCP_SRC_PORT, 
                               unsigned short lowerBoundTCPdstPort = DirectTCPProber::DEFAULT_LOWER_TCP_DST_PORT, 
                               unsigned short upperBoundTCPdstPort = DirectTCPProber::DEFAULT_UPPER_TCP_DST_PORT, 
                               bool verbose = false) throw(SocketException);
    virtual ~DirectTCPWrappedICMPProber();
    virtual ProbeRecord *basic_probe(const InetAddress &src, 
                                     const InetAddress &dst, 
                                     unsigned short IPIdentifier, 
                                     unsigned char TTL, 
                                     bool usingFixedFlowID, 
                                     unsigned short srcPort, 
                                     unsigned short dstPort, 
                                     bool recordeRoute = false, 
                                     InetAddress **looseSourceList = 0) throw(SocketSendException, SocketReceiveException);
};

#endif /* DIRECTTCPWRAPPEDICMPPROBER_H_ */
