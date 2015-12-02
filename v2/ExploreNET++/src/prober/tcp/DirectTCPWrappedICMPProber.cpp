/*
 * DirectTCPWrappedICMPProber.cpp
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * Slightly edited in September 2015 by J.-F. Grailet to improve coding style.
 */

#include "DirectTCPWrappedICMPProber.h"

#include <netinet/ip_icmp.h>

DirectTCPWrappedICMPProber::DirectTCPWrappedICMPProber(string &attentionMessage, 
                                                       int tcpUdpRoundRobinSocketCount, 
                                                       const TimeVal &timeoutPeriod, 
                                                       const TimeVal &probeRegulatorPausePeriod, 
                                                       unsigned short lowerBoundTCPsrcPort, 
                                                       unsigned short upperBoundTCPsrcPort, 
                                                       unsigned short lowerBoundTCPdstPort, 
                                                       unsigned short upperBoundTCPdstPort, 
                                                       bool verbose) throw(SocketException):
DirectTCPProber(attentionMessage, 
                tcpUdpRoundRobinSocketCount, 
                timeoutPeriod, 
                probeRegulatorPausePeriod, 
                lowerBoundTCPsrcPort, 
                upperBoundTCPsrcPort, 
                lowerBoundTCPdstPort, 
                upperBoundTCPdstPort, 
                verbose)
{

}

DirectTCPWrappedICMPProber::~DirectTCPWrappedICMPProber()
{
}

ProbeRecord *DirectTCPWrappedICMPProber::basic_probe(const InetAddress &src, 
                                                     const InetAddress &dst, 
                                                     unsigned short IPIdentifier, 
                                                     unsigned char TTL, 
                                                     bool usingFixedFlowID, 
                                                     unsigned short srcPort, 
                                                     unsigned short dstPort, 
                                                     bool recordeRoute, 
                                                     InetAddress **looseSourceList) throw(SocketSendException, SocketReceiveException)
{
    ProbeRecord *result = DirectTCPProber::basic_probe(src, 
                                                       dst, 
                                                       IPIdentifier, 
                                                       TTL, 
                                                       usingFixedFlowID, 
                                                       srcPort, 
                                                       dstPort, 
                                                       recordeRoute, 
                                                       looseSourceList);
    if(result->getRplyICMPtype() == DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE 
    || (result->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE 
    && result->getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE))
    {
        result->setRplyICMPtype(DirectProber::ICMP_TYPE_ECHO_REPLY);
        result->setRplyICMPcode(0);
    }
    return result;
}
