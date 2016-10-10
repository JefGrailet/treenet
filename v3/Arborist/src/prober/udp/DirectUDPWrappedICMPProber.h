/*
 * DirectUDPWrappedICMPProber.h
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * This class performs a probe just like DirectUDPProber but re-interprets the result as an ICMP 
 * Echo reply (because the reply to some UDP probe is typically a port unreachable reply) such 
 * that other parts of the program can easily interpret the result.
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -September 2015: slight edit to improve coding style.
 * -15 April 2016: added a method to rise a flag to use an unlikely high destination port number 
 *  inside the probes.
 * -Augustus 2016: removed an old test method. Also removed the loose source and record route 
 *  options, because they are unused by both TreeNET and ExploreNEt v2.1 and because the IETF (see 
 *  RFC 7126) reports that packets featuring these options are widely dropped, and that the 
 *  default policy of a router receiving such packets should be to drop them anyway due to 
 *  security concerns.
 */

#ifndef DIRECTUDPWRAPPEDICMPPROBER_H_
#define DIRECTUDPWRAPPEDICMPPROBER_H_

#include "DirectUDPProber.h"
#include "../../common/inet/InetAddress.h"
#include "../exception/SocketSendException.h"
#include "../exception/SocketReceiveException.h"
#include "../exception/SocketException.h"
#include "../structure/ProbeRecord.h"
#include "../../common/date/TimeVal.h"

class DirectUDPWrappedICMPProber : public DirectUDPProber
{
public:

    DirectUDPWrappedICMPProber(string &attentionMessage, 
                               int tcpUdpRoundRobinSocketCount, 
                               const TimeVal &timeoutPeriod=DirectProber::DEFAULT_TIMEOUT_PERIOD, 
                               const TimeVal &probeRegulatorPausePeriod=DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD, 
                               unsigned short lowerBoundUDPsrcPort = DirectUDPProber::DEFAULT_LOWER_UDP_SRC_PORT, 
                               unsigned short upperBoundUDPsrcPort = DirectUDPProber::DEFAULT_UPPER_UDP_SRC_PORT, 
                               unsigned short lowerBoundUDPdstPort = DirectUDPProber::DEFAULT_LOWER_UDP_DST_PORT, 
                               unsigned short upperBoundUDPdstPort = DirectUDPProber::DEFAULT_UPPER_UDP_DST_PORT, 
                               bool verbose = false) throw(SocketException);
    virtual ~DirectUDPWrappedICMPProber();
    virtual ProbeRecord *basic_probe(const InetAddress &src, 
                                     const InetAddress &dst, 
                                     unsigned short IPIdentifier, 
                                     unsigned char TTL, 
                                     bool usingFixedFlowID, 
                                     unsigned short srcPort, 
                                     unsigned short dstPort) throw(SocketSendException, SocketReceiveException);
   
    // Next lines are additions by J.-F. Grailet to implement some alias resolution method
    inline void useHighPortNumber() { this->usingHighPortNumber = true; }
    
private:
    
    bool usingHighPortNumber;

};

#endif /* DIRECTUDPWRAPPEDICMPPROBER_H_ */
