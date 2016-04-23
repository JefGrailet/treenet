/*
 * DirectUDPWrappedICMPProber.h
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * Modifications brought by J.-F. Grailet since ExploreNET v2.1:
 * -September 2015: slight edit to improve coding style.
 * -15 April 2016: added a method to rise a flag to use an unlikely high destination port number 
 *  inside the probes.
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
    static void test();
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
                                     unsigned short dstPort, 
                                     bool recordeRoute = false, 
                                     InetAddress **looseSourceList = 0) throw(SocketSendException, SocketReceiveException);
   
    // Next lines are additions by J.-F. Grailet to implement some alias resolution method
    inline void useHighPortNumber() { this->usingHighPortNumber = true; }
    
private:
    
    bool usingHighPortNumber;
};

#endif /* DIRECTUDPWRAPPEDICMPPROBER_H_ */
