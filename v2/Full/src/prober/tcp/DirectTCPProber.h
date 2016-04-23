/*
 * DirectTCPProber.h
 *
 *  Created on: Dec 13, 2009
 *      Author: root
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -September 2015: slight edit to improve coding style.
 * -March 4, 2016: slightly edited to comply to the (slightly) extended ProbeRecord class.
 */

#ifndef DIRECTTCPPROBER_H_
#define DIRECTTCPPROBER_H_

#define DEFAULT_DIRECT_TCP_PROBER_BUFFER_SIZE 512
#define DEFAULT_TCP_PSEUDO_HEADER_LENGTH 512

#include <memory>
using std::auto_ptr;

#include <cstdio>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <cstdlib>

#include "../DirectProber.h"
#include "../../common/inet/InetAddress.h"
#include "../exception/SocketSendException.h"
#include "../exception/SocketReceiveException.h"
#include "../exception/SocketException.h"
#include "../structure/ProbeRecord.h"
#include "../../common/date/TimeVal.h"

class DirectTCPProber : public DirectProber
{
public:
    static const unsigned short DEFAULT_LOWER_TCP_SRC_PORT;
    static const unsigned short DEFAULT_UPPER_TCP_SRC_PORT;
    static const unsigned short DEFAULT_LOWER_TCP_DST_PORT;
    static const unsigned short DEFAULT_UPPER_TCP_DST_PORT;

    static void test();

    DirectTCPProber(string &attentionMessage, 
                    int tcpUdpRoundRobinSocketCount, 
                    const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD, 
                    const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD, 
                    unsigned short lowerBoundTCPsrcPort = DirectTCPProber::DEFAULT_LOWER_TCP_SRC_PORT, 
                    unsigned short upperBoundTCPsrcPort = DirectTCPProber::DEFAULT_UPPER_TCP_SRC_PORT, 
                    unsigned short lowerBoundTCPdstPort = DirectTCPProber::DEFAULT_LOWER_TCP_DST_PORT, 
                    unsigned short upperBoundTCPdstPort = DirectTCPProber::DEFAULT_UPPER_TCP_DST_PORT, 
                    bool verbose = false) throw(SocketException);
    virtual ~DirectTCPProber();
    
    /**
     * Returns ICMP type DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE, code 
     * DirectProber::PSEUDO_TCP_RESET_ICMP_CODE in case a TCP RESET is obtained from the 
     * destination. Otherwise returns ICMP type and codes declared by IANA.
     */
    virtual ProbeRecord *basic_probe(const InetAddress &src, 
                                     const InetAddress &dst, 
                                     unsigned short IPIdentifier, 
                                     unsigned char TTL, 
                                     bool usingFixedFlowID, 
                                     unsigned short srcPort, 
                                     unsigned short dstPort, 
                                     bool recordeRoute = false, 
                                     InetAddress **looseSourceList = 0) throw(SocketSendException, SocketReceiveException);

protected:
    ProbeRecord *buildProbeRecord(const auto_ptr<TimeVal> &reqTime, 
                                  const InetAddress &dstAddress, 
                                  const InetAddress &rplyAddress, 
                                  unsigned char reqTTL, 
                                  unsigned char rplyTTL, 
                                  unsigned char replyType, 
                                  unsigned char rplyCode, 
                                  unsigned short srcIPidentifier, 
                                  unsigned short rplyIPidentifier,  
                                  unsigned char payloadTTL, 
                                  unsigned short payloadLength, 
                                  int probingCost, 
                                  bool usingFixedFlowID, 
                                  InetAddress *const RR = 0, 
                                  int RRlength = 0);
    uint8_t buffer[DEFAULT_DIRECT_TCP_PROBER_BUFFER_SIZE];
    uint8_t pseudoBuffer[DEFAULT_TCP_PSEUDO_HEADER_LENGTH];
};
#endif /* DIRECTTCPPROBER_H_ */
