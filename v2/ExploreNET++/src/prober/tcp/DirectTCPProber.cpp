/*
 * This file implements the class described in DirectTCPProber.h. Date at which it was implemented 
 * is unknown (though it should be the same as DirectTCPProber.h).
 * 
 * Edited by J.-F. Grailet in September 2015 to improve coding style.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <cerrno>

#include <iostream>
using std::cout;
using std::endl;

#include "DirectTCPProber.h"
#include "../../common/thread/Thread.h"

const unsigned short DirectTCPProber::DEFAULT_LOWER_TCP_SRC_PORT = 39000;
const unsigned short DirectTCPProber::DEFAULT_UPPER_TCP_SRC_PORT = 64000;
const unsigned short DirectTCPProber::DEFAULT_LOWER_TCP_DST_PORT = 39000;
const unsigned short DirectTCPProber::DEFAULT_UPPER_TCP_DST_PORT = 64000;

void DirectTCPProber::test()
{
    string attackMsg = "Generated for test purposes";
    DirectProber *pr = new DirectTCPProber(attackMsg,
                                           DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT,
                                           TimeVal(3, 0),
                                           DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD,
                                           DirectTCPProber::DEFAULT_LOWER_TCP_SRC_PORT,
                                           DirectTCPProber::DEFAULT_UPPER_TCP_SRC_PORT,
                                           DirectTCPProber::DEFAULT_LOWER_TCP_DST_PORT,
                                           DirectTCPProber::DEFAULT_UPPER_TCP_DST_PORT,
                                           true);

    InetAddress dst("www.mit.edu");
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
    
    for(int i = 1; i <= 1; i++)
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
        cout << endl << "Sleeping for 1 seconds before next batch" << endl;
        Thread::invokeSleep(TimeVal(1, 0));
    }
}

DirectTCPProber::DirectTCPProber(string &attentionMessage,
                                 int tcpUdpRoundRobinSocketCount,
                                 const TimeVal &timeoutPeriod,
                                 const TimeVal &probeRegulatorPausePeriod,
                                 unsigned short lowerBoundTCPsrcPort,
                                 unsigned short upperBoundTCPsrcPort,
                                 unsigned short lowerBoundTCPdstPort,
                                 unsigned short upperBoundTCPdstPort,
                                 bool verbose) throw (SocketException):
DirectProber(attentionMessage, 
             tcpUdpRoundRobinSocketCount, 
             IPPROTO_TCP, 
             timeoutPeriod, 
             probeRegulatorPausePeriod, 
             lowerBoundTCPsrcPort, 
             upperBoundTCPsrcPort, 
             lowerBoundTCPdstPort, 
             upperBoundTCPdstPort, 
             verbose)
{
}

DirectTCPProber::~DirectTCPProber()
{
    // TODO Auto-generated destructor stub
}

ProbeRecord *DirectTCPProber::basic_probe(const InetAddress &src,
                                          const InetAddress &dst,
                                          unsigned short IPIdentifier,
                                          unsigned char TTL,
                                          bool usingFixedFlowID,
                                          unsigned short srcPort,
                                          unsigned short dstPort,
                                          bool recordRoute,
                                          InetAddress **looseSourceList) throw (SocketSendException, SocketReceiveException)
{
    uint32_t src_32 = (uint32_t) (src.getULongAddress());
    uint32_t dst_32 = (uint32_t) (dst.getULongAddress());
    uint16_t IPIdentifier_16= (uint16_t) IPIdentifier;
    uint8_t TTL_8 = (uint8_t) TTL;
    uint16_t srcPort_16 = (uint16_t) srcPort;
    uint16_t dstPort_16 = (uint16_t) dstPort;

    uint32_t tcpSeq_32 = (uint32_t) rand();

    if(recordRoute == true && looseSourceList != 0)
    {
        string msg = "Enabling both record route and loose source addressing is discouraged ";
        msg += "because of security concerns. Please enable only one of these IP options.";
        throw SocketSendException(msg);
    }

    int looseSourceListSize = 0;
    InetAddress *currentAddress = 0;
    if(looseSourceList != 0)
    {
        for(currentAddress = looseSourceList[0]; currentAddress!=0 && looseSourceListSize < 9; currentAddress++)
        {
            looseSourceListSize++;
        }
    }

    // 1 prepare packet to send
    // Set ip fields
    uint32_t IPOptionsLength = 0; // Assumed to be zero by default (in bytes)
    uint32_t IPPaddingLength = 0; //
    if(looseSourceList != 0)
    {
        // Set the options length
        IPOptionsLength = (uint32_t) 4 * looseSourceListSize + 3;
        //3 comes from the type/length/offset-pointer of loose source routing option
    }
    else if(recordRoute == true)
    {
        // Set the options length
        IPOptionsLength = (uint32_t) 4 * DirectProber::DEFAULT_MAX_RECORD_ROUTE_ADDRESS_SIZE + 3;
        //3 comes from the type/length/offset-pointer of RR routing option
    }

    uint32_t IPHeaderLength = DirectProber::MINIMUM_IP_HEADER_LENGTH + IPOptionsLength;
    if(IPHeaderLength % 4 != 0)
    {
        IPPaddingLength = 4 - (IPHeaderLength % 4);
        IPHeaderLength += IPPaddingLength;
    }
    struct ip *ip = (struct ip*) buffer;
    ip->ip_v = DirectProber::DEFAULT_IP_VERSION;
    ip->ip_hl = IPHeaderLength / ((uint32_t)4); // In terms of 4 byte units
    ip->ip_tos = DirectProber::DEFAULT_IP_TOS;
    uint16_t totalPacketLength = IPHeaderLength + DirectProber::MINIMUM_TCP_HEADER_LENGTH;
    totalPacketLength += DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH + (uint32_t) getAttentionMsg().length();
    ip->ip_len = htons(totalPacketLength);
    ip->ip_id = htons(IPIdentifier_16);
    ip->ip_off = htons(DirectProber::DEFAULT_IP_FRAGMENT_OFFSET);
    ip->ip_ttl = TTL_8;
    ip->ip_p = IPPROTO_TCP;
    (ip->ip_src).s_addr = htonl(src_32);
    
    // Sets destination address
    if(looseSourceList != 0)
    {
        /**
         * If loose source routing is enabled than the very first destination must be (looseSourceList[0]) and the
         * last destination will be the dst argument!!!!
         */
         
        (ip->ip_dst).s_addr = htonl((uint32_t) ((looseSourceList[0])->getULongAddress()));
    }
    else
    {
        (ip->ip_dst).s_addr = htonl(dst_32);
    }
    
    // Sets options field
    uint8_t *optionsField = 0;
    if(looseSourceList != 0)
    {
        // Works for both LS and LSRR
        optionsField = buffer + DirectProber::MINIMUM_IP_HEADER_LENGTH;
        memset(optionsField++, 131, 1); // 131 is type of loose source routing
        memset(optionsField++, IPOptionsLength, 1);
        memset(optionsField++, 4,1); // 4 is the offset of the first address in options field
        uint32_t addr;

        for(int i = 1; i < looseSourceListSize; i++)
        {
            addr = htonl((uint32_t) (looseSourceList[i]->getULongAddress()));
            memcpy(optionsField, &addr, 4);
            optionsField += 4;
        }
        
        // Sets final destination to the end of options field
        addr = htonl(dst_32);
        memcpy(optionsField, &addr, 4);
        optionsField += 4;
    }
    else if(recordRoute == true)
    {
        optionsField=buffer+DirectProber::MINIMUM_IP_HEADER_LENGTH;
        memset(optionsField++, 7, 1); // 7 is type of record route
        memset(optionsField++, IPOptionsLength, 1);
        memset(optionsField++, 4, 1); // 4 is the offset of the first address in options field

        memset(optionsField, 0, DirectProber::DEFAULT_MAX_RECORD_ROUTE_ADDRESS_SIZE * (uint32_t) 4);
        optionsField += DirectProber::DEFAULT_MAX_RECORD_ROUTE_ADDRESS_SIZE * (uint32_t) 4;

    }

    if(IPPaddingLength>0)
    {
        memset(optionsField, 0, IPPaddingLength);
        optionsField += IPPaddingLength;
    }

    ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
    // Even though ip checksum is 2 bytes long, we don't need to apply htons() or ntohs() on this field
    ip->ip_sum = DirectProber::calculateInternetChecksum((uint16_t*) buffer, IPHeaderLength);

    // Sets tcp fields
    struct tcphdr *tcp = (struct tcphdr*) (buffer + IPHeaderLength);

    // memset((uint8_t*) tcp, 0, DirectProber::MINIMUM_TCP_HEADER_LENGTH);

    tcp->source = htons(srcPort_16);
    tcp->dest = htons(dstPort_16);
    tcp->seq = htonl(tcpSeq_32);
    tcp->ack_seq = htonl((unsigned long) rand());
    tcp->doff = (DirectProber::MINIMUM_TCP_HEADER_LENGTH / 4);
    tcp->res1 = 0;
    tcp->res2 = 0;
    tcp->syn = 1;
    tcp->ack = 1;
    tcp->fin = 0;
    tcp->psh = 0;
    tcp->urg = 0;
    tcp->rst = 0;
    tcp->window = htons(32767);
    tcp->check = 0x0;
    tcp->urg_ptr = 0x0;

    /**
     * Set the 8 bytes data portion of the TCP packet in order to keep a single flow
     * and avoid routers applying path balancing. The random data must be generated
     * and put into the randomDataBuffer by the calling function.
     */
    uint8_t *tcpdata = ((uint8_t*) tcp + DirectProber::MINIMUM_TCP_HEADER_LENGTH);
    memcpy(tcpdata, this->randomDataBuffer, DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH);
    tcpdata += DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH;
    memcpy(tcpdata, getAttentionMsg().c_str(), getAttentionMsg().length());
    tcpdata += getAttentionMsg().length();

    // TCP checksum is calculated over pseudo header
    uint8_t *pseudo=pseudoBuffer;
    memcpy(pseudo, &((ip->ip_src).s_addr), 4);
    pseudo += 4;
    memcpy(pseudo, &((ip->ip_dst).s_addr), 4);
    pseudo += 4;
    memset(pseudo++, 0, 1); // 1 byte padding
    memset(pseudo++, IPPROTO_TCP, 1); // 1 byte protocol
    uint16_t tcpPseudoLength = htons(DirectProber::MINIMUM_TCP_HEADER_LENGTH 
    + DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH + (uint32_t) getAttentionMsg().length());
    memcpy(pseudo, &tcpPseudoLength, 2);
    pseudo += 2;

    // Now the whole original TCP message is appended to the pseudoheader
    memcpy(pseudo, buffer, totalPacketLength);
    pseudo += totalPacketLength;

    int pseudoBufferLength = pseudo - pseudoBuffer;

    tcp->check=DirectProber::calculateInternetChecksum((uint16_t*) pseudoBuffer, pseudoBufferLength);
    
    // return 0;
    
    // 2) send the SYN+ACK packet
    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = (ip->ip_dst).s_addr;

    // Thread::invokeSleep(TimeVal(30,0));

    regulateProbingFrequency();
    if(verbose)
    {
        cout << "TCP-PROBING: src="<<src
        << " dst="<<dst
        << " IPidentifier="<<IPIdentifier
        << " TTL="<<(int)TTL
        << " srcPort="<<srcPort_16
        << " dstPort="<<dstPort_16
        << " sequenceNumber="<<ntohl(tcp->seq)
        << " acknowledgedSequenceNumber="<<ntohl(tcp->ack_seq)
        << " windowSize="<<ntohs(tcp->window);

        if(looseSourceList != 0)
        {
            cout << " LooseSourceList=";
            for(int i = 0; i < looseSourceListSize; i++)
            {
                cout << **(looseSourceList + i) << " ";
            }
        }
        
        if(recordRoute == true)
            cout << " RR=Enabled";
        else
            cout << " RR=Disabled";
        cout << endl;
    }

    ssize_t bytesSent = 0;
    ssize_t totalBytesSent = 0;
    do
    {
        bytesSent = sendto(this->sendSocketRAW,
                           this->buffer + totalBytesSent,
                           totalPacketLength - totalBytesSent,
                           0,
                           (struct sockaddr*) &to,
                           sizeof(struct sockaddr));
        if(bytesSent == -1)
        {
            perror("Socket Send Exception Error Message");
            throw SocketSendException("Can NOT send the TCP packet");
        }

        totalBytesSent += bytesSent;

    }
    while(totalBytesSent < totalPacketLength);
    auto_ptr<TimeVal> REQTime = TimeVal::getCurrentSystemTime();
    updateLastProbingTime();

    // 3) receives the reply packet
    struct sockaddr_in fromAddress;
    socklen_t fromAddressLength = sizeof(fromAddress);

    ssize_t receivedBytes = 0;

    TimeVal wait;
    while(1)
    {
        wait = (*REQTime) + timeout;
        auto_ptr<TimeVal> now = TimeVal::getCurrentSystemTime();
        wait -= (*now);

        if(wait.isUndefined())
        {
            wait.resetToZero();
        }
        
        //cout << "--------------------Wait " << (wait.isPositive() ? "POSITIVE " : "NEGATIVE or ZERO ") << wait << endl;

        RESET_SELECT_SET();
        int selectResult = select(getHighestSocketIdentifierForSelect() + 1, &receiveSet, NULL, NULL, wait.getStructure());
        
        //cout << "--------------------Wait " << (wait.isPositive() ? "POSITIVE " : "NEGATIVE or ZERO ") << wait << endl;
        //cout << "selectResult: " << selectResult << endl;

        if(selectResult > 0)
        {
            //************************  packet arrived   *****************

            // First, makes sure that the packet arrived to the socket we are interested in
            int readySocketDescriptor = GET_READY_SOCKET_DESCRIPTOR();
            if(readySocketDescriptor < 0)
            {
                continue;
            }

            /**
             * recvfrom method reads an entire packet into the buffer for SOCK_RAW, SOCK_DGRAM, 
             * and SOCK_SEQPACKET types of sockets, if the buffer is less than the packet length 
             * the rest of the packet is overflowing part is discarded. However, for SOCK_STREAM 
             * it reads in the portions of a packet as soon as it becomes available.
             *
             * Since our socket is SOCK_RAW, we can freely assume that the packet is received as 
             * a whole without worrying about packet portions received. However, though our socket 
             * receives only ICMP packets we must make sure that the version of the received 
             * packet is IPv4 and the received packet is a response to the packet that we sent.
             */

            receivedBytes = recvfrom(readySocketDescriptor, // socket handle
                                    this->buffer, // receive buffer to store the received packets
                                    DEFAULT_DIRECT_TCP_PROBER_BUFFER_SIZE, // receive buffer size in bytes
                                    0, // flags
                                    (struct sockaddr*) &fromAddress,
                                    &fromAddressLength);

            if(receivedBytes == -1)
            {
                perror("Socket Receive Exception Error Message");
                throw SocketReceiveException("Can NOT receive packets");
            }

            // Resets ip for the arrived packet
            ip = (struct ip*) buffer;

            /**
             * The very first byte received holds the incoming packet's IP version and IP header 
             * length, two 4 bit length fields. If the IP version is not IPv4 or IP header length 
             * is less than the minimum header length we ignore this packet and continue receiving 
             * for the next packet.
             */

            if(ip->ip_v != DirectProber::DEFAULT_IP_VERSION)
            {
                if(verbose)
                {
                    cout << "CONTINUE RECEIVING: Received packet is not IPv4" << endl;
                }
                continue;
            }

            /**
             * We know that the packets of SOCK_RAW received ad whole in contrast to SOCK_STREAM
             * However, we do a check here and if it is not correct the packet would be dropped
             * because we set totalReceivedBytes=0; and call select() and receive methods again.
             */

            if(receivedBytes < DirectProber::MINIMUM_IP_HEADER_LENGTH)
            {
                if(verbose)
                {
                    cout << "CONTINUE RECEIVING: Received bytes are less than the minimum header length" << endl;
                }
                continue;
            }

            uint16_t receivedIPtotalLength = ntohs(ip->ip_len);
            uint16_t receivedIPheaderLength = ((uint16_t) ip->ip_hl) * (uint16_t) 4;

            if(receivedBytes < receivedIPtotalLength)
            {
                if(verbose)
                {
                    cout << "CONTINUE RECEIVING: Received bytes are less than the specified header length" << endl;
                }
                continue;
            }

            // Computes the IP header checksum to make sure that the IP header is intact
            // Despite ip header checksum is 2 bytes long we don't need to apply htons() or ntohs()
            uint16_t tmpChecksum = ip->ip_sum;
            
            // Temporarily sets ip_sum to zero in order to compute checksum. This value will be restored later
            ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
            if(DirectProber::calculateInternetChecksum((uint16_t*) buffer, receivedIPheaderLength) != tmpChecksum)
            {
                if(verbose)
                {
                    cout << "CONTINUE RECEIVING: IP header checksum error" << endl;
                }
                continue;
            }
            ip->ip_sum = tmpChecksum;

            uint8_t IPProtocol = ip->ip_p;
            if(IPProtocol != IPPROTO_ICMP && IPProtocol != IPPROTO_TCP)
            {
                if(verbose)
                {
                    cout << "CONTINUE RECEIVING: Received packet is neither ICMP nor TCP" << endl;
                }
                continue;
            }

            if(IPProtocol == IPPROTO_ICMP)
            {
                /**
                 * Now we are sure that we got an ICMP packet of IPv4. Lets check the content of 
                 * the packet to see if this packet is a reply to the packet we sent out.
                 */

                // Resets icmp for the arrived packet
                struct icmphdr *icmp = (struct icmphdr*) (buffer + (ip->ip_hl) * 4);

                // First check is the ICMP checksum of the received packet is correct
                // Despite icmp checksum is 2 bytes long we don't need to apply htons() or ntohs()
                tmpChecksum = icmp->checksum;
                icmp->checksum = 0x0; // Before computing checksum, the sum field must be zero
                
                // We don't need to convert icmp checksum to network-byte-order
                if(DirectProber::calculateInternetChecksum((uint16_t*) icmp, receivedIPtotalLength - receivedIPheaderLength) != tmpChecksum)
                {
                    if(verbose)
                    {
                        cout << "CONTINUE RECEIVING: IP header checksum error" << endl;
                    }
                    continue;
                }
                icmp->checksum = tmpChecksum;

                if(icmp->type == DirectProber::ICMP_TYPE_TIME_EXCEEDED || icmp->type == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE)
                {
                    // Sets payload ip and icmp for time exceeded and unreachable replies.
                    struct ip *payloadip = (struct ip*) (buffer + receivedIPheaderLength + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
                    struct tcphdr *payloadtcp = (struct tcphdr*) (buffer + receivedIPheaderLength + 
                    DirectProber::DEFAULT_ICMP_HEADER_LENGTH + (payloadip->ip_hl)*4);
                    
                    /**
                     * We used to compare the returned 8 bytes field of payload message (the IP 
                     * header plus first 8 bytes of our message) of TTL exceeded messages. 
                     * However, when record-route enabled routers only return IP header NOT first 
                     * 8 bytes of the sent packet. Hence, we removed that part. But still we 
                     * generate 8 bytes and put into the outgoing packets because it minimizes 
                     * the risk of being considered as an attacker.
                     *
                     * uint8_t *startPayload8bytesData = (buffer + receivedIPheaderLength + 
                     * DirectProber::DEFAULT_ICMP_HEADER_LENGTH + (payloadip->ip_hl) * 4 + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
                     * for(int i = 0; i < 8; i++)
                     * {
                     *    cout << (int) startPayload8bytesData[i] << " ";
                     * }
                     * cout << endl;
                     */

                    cout << "payloadip->ip_p==IPPROTO_TCP: " << (payloadip->ip_p == IPPROTO_TCP) << endl;
                    cout << "ntohs(payloadip->ip_id)==IPIdentifier_16: "<< (ntohs(payloadip->ip_id) == IPIdentifier_16) << endl;
                    cout << "ntohs(payloadtcp->source)==srcPort_16: " << (ntohs(payloadtcp->source) == srcPort_16) << endl;
                    cout << "ntohs(payloadtcp->dest)==dstPort_16: " << (ntohs(payloadtcp->dest) == dstPort_16) << endl;
                    cout << "ntohl(payloadtcp->seq)==tcpSeq_32): " << (ntohl(payloadtcp->seq) == tcpSeq_32) << endl;

                    // IP identifier of the piggybacked packet is checked especially for multiplexing usingFixedFlowID packets
                    if(payloadip->ip_p == IPPROTO_TCP && ntohs(payloadip->ip_id) == IPIdentifier_16 
                    && ntohs(payloadtcp->source) == srcPort_16 && (ntohs(payloadtcp->dest) == dstPort_16 
                    || ntohl(payloadtcp->seq) == tcpSeq_32))
                    {
                        // This is the reply of the packet that we sent
                        InetAddress rplyAddress((unsigned long int) ntohl((ip->ip_src).s_addr));
                        if(recordRoute == true)
                        {
                            // We expect some routers on the way filled the options record route field
                            if((uint16_t) 4 * payloadip->ip_hl > DirectProber::MINIMUM_IP_HEADER_LENGTH)
                            {
                                uint8_t *startPayloadOptions = ((uint8_t*) (payloadip)) + DirectProber::MINIMUM_IP_HEADER_LENGTH;
                                // 7 means record route
                                if(*startPayloadOptions == 7)
                                {
                                    // 3 is type optionslength and offset
                                    uint32_t *RRIPValueStart = (uint32_t*) (startPayloadOptions + 3);
                                    uint32_t *RRIPValueEnd = ((uint32_t*) (startPayloadOptions + *(startPayloadOptions + 2))) - 1;
                                    int RRarrayLength = RRIPValueEnd - RRIPValueStart + 1;
                                    InetAddress *RRarray = new InetAddress[RRarrayLength];
                                    RRarray[RRIPValueEnd - RRIPValueStart] = 0;
                                    for(int i = 0; RRIPValueStart < RRIPValueEnd; i++, RRIPValueStart++)
                                    {
                                        RRarray[i].setInetAddress((unsigned long) ntohl(*RRIPValueStart));
                                    }
                                    return buildProbeRecord(REQTime, 
                                                            dst, 
                                                            rplyAddress, 
                                                            TTL, 
                                                            ip->ip_ttl, 
                                                            icmp->type, 
                                                            icmp->code, 
                                                            ntohs(ip->ip_id), 
                                                            payloadip->ip_ttl, 
                                                            1, 
                                                            usingFixedFlowID, 
                                                            RRarray, 
                                                            RRarrayLength);

                                }
                                else
                                {
                                    // Some weird thing happened: even though we asked for IP options we could not get that
                                }

                            // uint3_t2 *RRIPStart = (((uint8_t*) (payloadip)) + DirectProber::MINIMUM_IP_HEADER_LENGTH + )
                            }
                            else
                            {
                                // Some weird thing happened: even though we asked for IP options we could not get that
                            }

                        }
                        return buildProbeRecord(REQTime, 
                                                dst, 
                                                rplyAddress, 
                                                TTL, 
                                                ip->ip_ttl, 
                                                icmp->type, 
                                                icmp->code, 
                                                ntohs(ip->ip_id), 
                                                payloadip->ip_ttl, 
                                                1, 
                                                usingFixedFlowID, 
                                                0, 
                                                0);
                    }
                }
                else
                {
                    if(verbose)
                    {
                        cout << "UNKNOWN PACKET HAS BEEN RECEIVED" << endl;
                    }
                }
            }
            else if(IPProtocol == IPPROTO_TCP)
            {
                /**
                 * Now we are sure that we got a TCP packet of IPv4. Let us check the content of 
                 * the packet to see if this packet is a reply to the packet we sent out.
                 */

                // Reset ICMP for the arrived packet
                struct tcphdr *tcp = (struct tcphdr*) (buffer + (ip->ip_hl) * 4);

                if(ntohs(tcp->dest) == srcPort_16 && (ntohs(tcp->source) == dstPort_16 
                || ntohl(tcp->ack_seq) == tcpSeq_32 + 1 + DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH 
                + (uint32_t) getAttentionMsg().length()))
                {
                    // This is the reply of the packet that we sent
                    InetAddress rplyAddress((unsigned long int) ntohl((ip->ip_src).s_addr));
                    return buildProbeRecord(REQTime, 
                                            dst, 
                                            rplyAddress, 
                                            TTL, 
                                            ip->ip_ttl, 
                                            DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE, 
                                            DirectProber::PSEUDO_TCP_RESET_ICMP_CODE, 
                                            ntohs(ip->ip_id), 
                                            0, 
                                            1, 
                                            usingFixedFlowID, 
                                            0, 
                                            0);
                }
            }
        }
        //****************   Select timeout occured   *********************
        else if(selectResult == 0)
        {
            if(verbose)
            {
                cout << "select(...) function timed out" << endl;
            }
            return buildProbeRecord(REQTime, 
                                    dst, 
                                    InetAddress(0), 
                                    TTL, 
                                    0, 
                                    255, 
                                    255, 
                                    0, 
                                    0, 
                                    1, 
                                    usingFixedFlowID, 
                                    0, 
                                    0);
        }
        //****************   Select error occured    *******************
        else
        {
            if(verbose)
            {
                if(errno == EINVAL)
                    cout << "******select(...)******* EINVAL" << endl;
                else if(errno == EINTR)
                    cout << "******select(...)******* EINTR" << endl;
                else if(errno == EBADF)
                    cout << "******select(...)******* EBADF" << endl;
                else
                    cout << "******select(...)******* errno: " << errno << endl;
            }
            perror("select(...)");
            throw SocketReceiveException("Can NOT select() on receiving socket");
            return 0; // To make the compiler happy
        }
    } // End of while(1){
}

ProbeRecord *DirectTCPProber::buildProbeRecord(const auto_ptr<TimeVal> &reqTime, 
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
                                               InetAddress *const RR, 
                                               int RRlength)
{
    ProbeRecord *recordPtr = new ProbeRecord();
    recordPtr->setReqTime(*reqTime);
    recordPtr->setRplyTime(*(TimeVal::getCurrentSystemTime()));
    recordPtr->setDstAddress(dstAddress);
    recordPtr->setRplyAddress(rplyAddress);
    recordPtr->setReqTTL(reqTTL);
    recordPtr->setRplyTTL(rplyTTL);
    recordPtr->setRplyICMPtype(replyType);
    recordPtr->setRplyICMPcode(rplyCode);
    recordPtr->setRplyIPidentifier(rplyIPidentifier);
    recordPtr->setPayloadTTL(payloadTTL);
    recordPtr->setProbingCost(probingCost);
    recordPtr->setUsingFixedFlowID(usingFixedFlowID);
    recordPtr->setRR(RR);
    recordPtr->setRRlength(RRlength);
    if(verbose)
    {
        cout << "TCP-PROBING-RESPONSE:"
        << " dstAddress=" << recordPtr->getDstAddress() 
        << " rplyAddress=" << recordPtr->getRplyAddress() 
        << " reqTTL=" << (int) recordPtr->getReqTTL() 
        << " rplyIPidentifier=" << recordPtr->getRplyIPidentifier() 
        << " rplyICMPtype=" << (int) recordPtr->getRplyICMPtype() 
        << " rplyICMPcode=" << (int) recordPtr->getRplyICMPcode() 
        << " rplyTTL=" << (int) recordPtr->getRplyTTL() 
        << " payloadTTL=" << (int) recordPtr->getPayloadTTL();
        if(RR != 0)
            cout << " RR[" << RRlength << "]=";
        for(int i = 0; i < RRlength; i++)
            cout<<RR[i]<<" ";
        cout << " reqTime=" << recordPtr->getReqTime() 
        << " rplyTime=" << recordPtr->getRplyTime();
        cout << endl;
    }
    return recordPtr;
}
