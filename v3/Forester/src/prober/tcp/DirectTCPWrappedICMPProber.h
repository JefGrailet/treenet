/*
 * DirectTCPWrappedICMPProber.h
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * This class performs a probe just like DirectTCPProber but re-interprets the result as an ICMP 
 * Echo reply (because the reply to some TCP probe is typically a port unreachable reply or a TCP 
 * Reset message) such that other parts of the program can easily interpret the result.
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -September 2015: slight edit to improve coding style.
 * -March 4, 2016: slightly edited to comply to the (slightly) extended ProbeRecord class.
 * -Augustus 2016: removed the loose source and record route options, because they are unused by 
 *  both TreeNET and ExploreNEt v2.1 and because the IETF (see RFC 7126) reports that packets 
 *  featuring these options are widely dropped, and that the default policy of a router receiving 
 *  such packets should be to drop them anyway due to security concerns.
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
                                     unsigned short dstPort) throw(SocketSendException, SocketReceiveException);
};

#endif /* DIRECTTCPWRAPPEDICMPPROBER_H_ */
