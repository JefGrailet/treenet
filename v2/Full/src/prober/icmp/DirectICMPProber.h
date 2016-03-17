/*
 * DirectICMPProber.h
 *
 *  Created on: Jul 24, 2008
 *      Author: root
 *
 * Coding style slightly edited by Jean-François Grailet in December 2014.
 *
 * March 4, 2016: slightly edited to comply to the (slightly) extended ProbeRecord class.
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

	static void test();
	
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
                                     unsigned short ICMPsequence,
                                     bool recordeRoute = false,
                                     InetAddress **looseSourceList = 0)throw (SocketSendException, SocketReceiveException);
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
	                              int probingCost, 
	                              bool usingFixedFlowID, 
	                              InetAddress* const RR = 0, 
	                              int RRlength = 0);
	uint8_t buffer[DEFAULT_DIRECT_ICMP_PROBER_BUFFER_SIZE];
};

#endif /* DIRECTICMPPROBER_H_ */
