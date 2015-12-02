/*
 * DirectUDPProber.h
 *
 *  Created on: Jul 26, 2008
 *      Author: root
 *
 * Slightly edited in September 2015 by J.-F. Grailet to improve coding style.
 */

#ifndef DIRECTUDPPROBER_H_
#define DIRECTUDPPROBER_H_

#define DEFAULT_DIRECT_UDP_PROBER_BUFFER_SIZE 512
#define DEFAULT_UDP_PSEUDO_HEADER_LENGTH 512

#include <memory>
using std::auto_ptr;

#include "../DirectProber.h"
#include "../../common/inet/InetAddress.h"
#include "../exception/SocketSendException.h"
#include "../exception/SocketReceiveException.h"
#include "../exception/SocketException.h"
#include "../structure/ProbeRecord.h"
#include "../../common/date/TimeVal.h"

class DirectUDPProber: public DirectProber
{
public:
    static const unsigned short DEFAULT_LOWER_UDP_SRC_PORT;
    static const unsigned short DEFAULT_UPPER_UDP_SRC_PORT;
    static const unsigned short DEFAULT_LOWER_UDP_DST_PORT;
    static const unsigned short DEFAULT_UPPER_UDP_DST_PORT;

    static void test();

    /**
     * This class is responsible for sending a single UDP packet to a destination port number 
     * and get the reply.
     *
     * @param timeoutPeriod is in seconds
     */
    
    DirectUDPProber(string &attentionMessage, 
                    int tcpUdpRoundRobinSocketCount, 
                    const TimeVal &timeoutPeriod=DirectProber::DEFAULT_TIMEOUT_PERIOD, 
                    const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD, 
                    unsigned short lowerBoundUDPsrcPort = DirectUDPProber::DEFAULT_LOWER_UDP_SRC_PORT, 
                    unsigned short upperBoundUDPsrcPort = DirectUDPProber::DEFAULT_UPPER_UDP_SRC_PORT, 
                    unsigned short lowerBoundUDPdstPort = DirectUDPProber::DEFAULT_LOWER_UDP_DST_PORT, 
                    unsigned short upperBoundUDPdstPort = DirectUDPProber::DEFAULT_UPPER_UDP_DST_PORT, 
                    bool verbose = false) throw(SocketException);
    virtual ~DirectUDPProber();
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
                                  unsigned short rplyIPidentifier, 
                                  unsigned char payloadTTL, 
                                  int probingCost, 
                                  bool usingFixedFlowID, 
                                  InetAddress *const RR = 0, 
                                  int RRlength = 0);
    uint8_t buffer[DEFAULT_DIRECT_UDP_PROBER_BUFFER_SIZE];
    uint8_t pseudoBuffer[DEFAULT_UDP_PSEUDO_HEADER_LENGTH];
};

#endif /* DIRECTUDPPROBER_H_ */
