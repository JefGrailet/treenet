/*
 * DirectUDPProber.h
 *
 *  Created on: Jul 26, 2008
 *      Author: root
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -September 2015: slight edit to improve coding style.
 * -March 4, 2016: slightly edited to comply to the (slightly) extended ProbeRecord class.
 * -Augustus 2016: removed an old test method and added the new debug mechanics of TreeNET v3.0. 
 *  Also removed the loose source and record route options, because they are unused by both 
 *  TreeNET and ExploreNEt v2.1 and because the IETF (see RFC 7126) reports that packets featuring 
 *  these options are widely dropped, and that the default policy of a router receiving such 
 *  packets should be to drop them anyway due to security concerns.
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

    /**
     * This class is responsible for sending a single UDP packet to a destination port number 
     * and get the reply.
     *
     * @param timeoutPeriod is in seconds
     */
    
    DirectUDPProber(string &attentionMessage, 
                    int tcpUdpRoundRobinSocketCount, 
                    const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD, 
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
                                     unsigned short dstPort) throw(SocketSendException, SocketReceiveException);

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
                                  bool usingFixedFlowID);
    
    uint8_t buffer[DEFAULT_DIRECT_UDP_PROBER_BUFFER_SIZE];
    uint8_t pseudoBuffer[DEFAULT_UDP_PSEUDO_HEADER_LENGTH];

};

#endif /* DIRECTUDPPROBER_H_ */
