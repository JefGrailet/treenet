/*
 * DirectUDPWrappedICMPProber.cpp
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 *
 * Slightly edited in September 2015 by J.-F. Grailet to improve coding style.
 */

#include "DirectUDPWrappedICMPProber.h"
#include "../../common/thread/Thread.h"

#include <netinet/ip_icmp.h>

void DirectUDPWrappedICMPProber::test()
{
    string attackMsg = "Generated for test purposes";
    DirectProber *pr = new DirectUDPWrappedICMPProber(attackMsg, 
                                                      DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT, 
                                                      DirectProber::DEFAULT_TIMEOUT_PERIOD, 
                                                      DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD, 
                                                      DirectUDPProber::DEFAULT_LOWER_UDP_SRC_PORT, 
                                                      DirectUDPProber::DEFAULT_UPPER_UDP_SRC_PORT, 
                                                      DirectUDPProber::DEFAULT_LOWER_UDP_DST_PORT, 
                                                      DirectUDPProber::DEFAULT_UPPER_UDP_DST_PORT, 
                                                      true);

    InetAddress dst("www.fatih.edu.tr");
    InetAddress src = InetAddress::getFirstLocalAddress();
    ProbeRecord *rec = 0;
    int consecutiveAnonymousCount = 0;
    static unsigned char CONJECTTURED_INTERNET_DIAMETER = 48;
    unsigned char TTL = 1;
    
    /**
     * When i = 1, tests single probe with wireshark
     * When i = 2, tests double probe with wireshark
     * When i = 3, tests record route with wireshark
     */
    
    for(int i = 1; i <= 3; i++)
    {
        consecutiveAnonymousCount = 0;
        TTL = 1;
        if(i == 1)
            cout << endl << endl << "TESTING SINGLE PROBE" << endl;
        else if(i == 2)
            cout << endl << endl << "TESTING DOUBLE PROBE" << endl;
        else
            cout << endl << endl << "TESTING SINGLE PROBE WITH RECORD ROUTE" << endl;

        do
        {
            if(i == 1)
                rec = pr->singleProbe(src, dst, TTL, false, 0, 0);
            else if(i == 2)
                rec = pr->doubleProbe(src, dst, TTL, false, 0, 0);
            else
                rec = pr->singleProbe(src, dst, TTL, false, true, 0);

            if(rec->isAnonymousRecord())
                consecutiveAnonymousCount++;
            else
                consecutiveAnonymousCount = 0;
            
            cout << "[" << (TTL < 10 ? "0" : "") << (int) TTL << "]" << *rec << endl << endl;
            if(!rec->isAnonymousRecord() && rec->getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                delete rec;
                rec = 0;
                break;
            }
            delete rec;
            rec = 0;
            TTL++;
        }
        while(consecutiveAnonymousCount < 4 && TTL < CONJECTTURED_INTERNET_DIAMETER);
        cout << endl << "Sleeping for 5 seconds before next batch" << endl;
        Thread::invokeSleep(TimeVal(5, 0));
    }
}

DirectUDPWrappedICMPProber::DirectUDPWrappedICMPProber(string &attentionMessage, 
                                                       int tcpUdpRoundRobinSocketCount, 
                                                       const TimeVal &timeoutPeriod, 
                                                       const TimeVal &probeRegulatorPausePeriod, 
                                                       unsigned short lowerBoundUDPsrcPort, 
                                                       unsigned short upperBoundUDPsrcPort, 
                                                       unsigned short lowerBoundUDPdstPort, 
                                                       unsigned short upperBoundUDPdstPort, 
                                                       bool verbose) throw(SocketException):
DirectUDPProber(attentionMessage, 
                tcpUdpRoundRobinSocketCount, 
                timeoutPeriod, 
                probeRegulatorPausePeriod, 
                lowerBoundUDPsrcPort, 
                upperBoundUDPsrcPort, 
                lowerBoundUDPdstPort, 
                upperBoundUDPdstPort, 
                verbose)
{

}

DirectUDPWrappedICMPProber::~DirectUDPWrappedICMPProber()
{
    // TODO Auto-generated destructor stub
}

ProbeRecord *DirectUDPWrappedICMPProber::basic_probe(const InetAddress &src, 
                                                     const InetAddress &dst, 
                                                     unsigned short IPIdentifier, 
                                                     unsigned char TTL, 
                                                     bool usingFixedFlowID, 
                                                     unsigned short srcPort, 
                                                     unsigned short dstPort, 
                                                     bool recordeRoute, 
                                                     InetAddress **looseSourceList) throw(SocketSendException, SocketReceiveException)
{
    ProbeRecord *result = DirectUDPProber::basic_probe(src, 
                                                       dst, 
                                                       IPIdentifier, 
                                                       TTL, 
                                                       usingFixedFlowID, 
                                                       srcPort, 
                                                       dstPort, 
                                                       recordeRoute, 
                                                       looseSourceList);
    
    if(result->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE)
    {
        if(result->getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE)
        {
            result->setRplyICMPtype(DirectProber::ICMP_TYPE_ECHO_REPLY);
            result->setRplyICMPcode(0);
        }
    }
    return result;
}
