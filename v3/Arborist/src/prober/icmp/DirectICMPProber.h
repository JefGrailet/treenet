/*
 * DirectICMPProber.h
 *
 *  Created on: Jul 24, 2008
 *      Author: root
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -December 2014: slight edit to improve coding style.
 * -March 4, 2016: slightly edited to comply to the (slightly) extended ProbeRecord class.
 * -Augustus 2016: updated the debug mode, to feature all details into a small log, and removed 
 *  an old test method (still in TreeNET v2.3). Also removed the loose source and record route 
 *  options, because they are unused by both TreeNET and ExploreNEt v2.1 and because the IETF (see 
 *  RFC 7126) reports that packets featuring these options are widely dropped, and that the 
 *  default policy of a router receiving such packets should be to drop them anyway due to 
 *  security concerns.
 */

#ifndef DIRECTICMPPROBER_H_
#define DIRECTICMPPROBER_H_

#define DEFAULT_DIRECT_ICMP_PROBER_BUFFER_SIZE 512

#include <memory>
using std::auto_ptr;
#include <inttypes.h>

#include "../DirectProber.h"
#include "../../common/inet/InetAddress.h"
#include "../exception/SocketSendException.h"
#include "../exception/SocketReceiveException.h"
#include "../exception/SocketException.h"
#include "../structure/ProbeRecord.h"
#include "../../common/date/TimeVal.h"

class DirectICMPProber : public DirectProber
{
public:

    static const unsigned short DEFAULT_LOWER_ICMP_IDENTIFIER;
    static const unsigned short DEFAULT_UPPER_ICMP_IDENTIFIER;
    static const unsigned short DEFAULT_LOWER_ICMP_SEQUENCE;
    static const unsigned short DEFAULT_UPPER_ICMP_SEQUENCE;

    /**
     * This class is responsible for sending a single ICMP echo request message and get the reply
     * @param timeoutPeriod is in seconds
     */
    
    DirectICMPProber(string &attentionMessage,
                     const TimeVal &timeoutPeriod = DirectProber::DEFAULT_TIMEOUT_PERIOD,
                     const TimeVal &probeRegulatorPausePeriod = DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
                     unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                     unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                     unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                     unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE,
                     bool verbose = false) throw(SocketException);
    virtual ~DirectICMPProber();
    virtual ProbeRecord *basic_probe(const InetAddress &src, 
                                     const InetAddress &dst, 
                                     unsigned short IPIdentifier, 
                                     unsigned char TTL, 
                                     bool usingFixedFlowID, 
                                     unsigned short ICMPidentifier, 
                                     unsigned short ICMPsequence) throw (SocketSendException, SocketReceiveException);
    
    // Addition by J.-F. Grailet (up to "private:" part included) to implement Timestamp request
    inline void useTimestampRequests() { this->usingTimestampRequests = true; }
    
private:
    
    bool usingTimestampRequests;

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
                                  unsigned long originateTs, 
                                  unsigned long receiveTs, 
                                  unsigned long transmitTs, 
                                  int probingCost, 
                                  bool usingFixedFlowID);
    
    uint8_t buffer[DEFAULT_DIRECT_ICMP_PROBER_BUFFER_SIZE];
    
};

#endif /* DIRECTICMPPROBER_H_ */
