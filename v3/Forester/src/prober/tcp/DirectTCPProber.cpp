/*
 * This file implements the class described in DirectTCPProber.h. Date at which it was implemented 
 * is unknown (though it should be the same as DirectTCPProber.h).
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <cerrno>

#include <sstream>
using std::stringstream;

#include <iostream>

#include "DirectTCPProber.h"
#include "../../common/thread/Thread.h"

const unsigned short DirectTCPProber::DEFAULT_LOWER_TCP_SRC_PORT = 39000;
const unsigned short DirectTCPProber::DEFAULT_UPPER_TCP_SRC_PORT = 64000;
const unsigned short DirectTCPProber::DEFAULT_LOWER_TCP_DST_PORT = 39000;
const unsigned short DirectTCPProber::DEFAULT_UPPER_TCP_DST_PORT = 64000;

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
             IPPROTO_TCP, 
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
                                          unsigned short dstPort) throw (SocketSendException, SocketReceiveException)
{
    uint32_t src_32 = (uint32_t) (src.getULongAddress());
    uint32_t dst_32 = (uint32_t) (dst.getULongAddress());
    uint16_t IPIdentifier_16 = (uint16_t) IPIdentifier;
    uint8_t TTL_8 = (uint8_t) TTL;
    uint16_t srcPort_16 = (uint16_t) srcPort;
    uint16_t dstPort_16 = (uint16_t) dstPort;

    this->nbProbes++;

    // 1) Prepares packet to send
    uint32_t IPHeaderLength = DirectProber::MINIMUM_IP_HEADER_LENGTH;
    struct ip *ip = (struct ip*) buffer;
    ip->ip_v = DirectProber::DEFAULT_IP_VERSION;
    ip->ip_hl = IPHeaderLength / ((uint32_t) 4); // In terms of 4 byte units
    ip->ip_tos = DirectProber::DEFAULT_IP_TOS;
    uint16_t totalPacketLength = IPHeaderLength + DirectProber::MINIMUM_TCP_HEADER_LENGTH;
    ip->ip_len = htons(totalPacketLength);
    ip->ip_id = htons(IPIdentifier_16);
    ip->ip_off = htons(DirectProber::DEFAULT_IP_FRAGMENT_OFFSET);
    ip->ip_ttl = TTL_8;
    ip->ip_p = IPPROTO_TCP;
    (ip->ip_src).s_addr = htonl(src_32);
    (ip->ip_dst).s_addr = htonl(dst_32);

    ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
    
    // Even though ip checksum is 2 bytes long, we don't need to apply htons() or ntohs() on this field
    ip->ip_sum = DirectProber::calculateInternetChecksum((uint16_t*) buffer, IPHeaderLength);

    // Sets tcp fields
    struct tcphdr *tcp = (struct tcphdr*) (buffer + IPHeaderLength);

    memset((uint8_t*) tcp, 0, DirectProber::MINIMUM_TCP_HEADER_LENGTH);
    tcp->source = htons(srcPort_16);
    tcp->dest = htons(dstPort_16);
    tcp->doff = (DirectProber::MINIMUM_TCP_HEADER_LENGTH / 4);
    tcp->syn = 1; // TCP SYN probe
    tcp->window = htons(32767);
    
    /*
     * N.B.: ACK, SYN+ACK probes are also a possibility, but are likely to give much less answers 
     * than SYN probes. More generally, TCP probing with TreeNET has a limited interest and should 
     * only be used in unique situations, as TreeNET heavily relies on target responsivity, which 
     * is difficult to obtain with ACK/SYN+ACK probes.
     */
    
    // TCP checksum is calculated over pseudo header
    uint8_t *pseudo = pseudoBuffer;
    memcpy(pseudo, &((ip->ip_src).s_addr), 4);
    pseudo += 4;
    memcpy(pseudo, &((ip->ip_dst).s_addr), 4);
    pseudo += 4;
    memset(pseudo++, 0, 1); // 1 byte padding
    memset(pseudo++, IPPROTO_TCP, 1); // 1 byte protocol
    uint16_t tcpPseudoLength = htons(DirectProber::MINIMUM_TCP_HEADER_LENGTH);
    memcpy(pseudo, &tcpPseudoLength, 2);
    pseudo += 2;

    // Now the whole original TCP message is appended to the pseudoheader
    memcpy(pseudo, buffer, totalPacketLength);
    pseudo += totalPacketLength;

    int pseudoBufferLength = pseudo - pseudoBuffer;
    tcp->check = DirectProber::calculateInternetChecksum((uint16_t*) pseudoBuffer, pseudoBufferLength);
    
    // 2) Sends the packet
    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = (ip->ip_dst).s_addr;

    regulateProbingFrequency();
    if(verbose)
    {
        stringstream logStream;
    
        // The socket identifiers are written before anything else
        logStream << "\n[SSID = " << this->sendSocketRAW;
        logStream << ", Port = " << this->tcpudpReceivePorts[this->activeTCPUDPReceiveSocketIndex];
        logStream << ", RSID = " << this->tcpudpReceiveSockets[this->activeTCPUDPReceiveSocketIndex] << "]\n";

        // Actual details on the probe.
        logStream << "TCP probing:\n";
        logStream << "Source address: " << src << "\n";
        logStream << "Destination address: " << dst << "\n";
        logStream << "IP identifier (source): " << IPIdentifier << "\n";
        logStream << "Initial TTL: " << (int) TTL << "\n";
        logStream << "Source port: " << srcPort_16 << "\n";
        logStream << "Destination port: " << dstPort_16 << "\n";
        logStream << "Sequence number: " << ntohl(tcp->seq) << "\n";
        logStream << "ACK sequence number: " << ntohl(tcp->ack_seq) << "\n";
        logStream << "Window size: " << ntohs(tcp->window) << "\n";

        this->log += logStream.str();
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
    
    if(verbose)
    {
        this->log += "Started listening for a reply...\n";
    }

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

        RESET_SELECT_SET();
        int selectResult = select(getHighestSocketIdentifierForSelect() + 1, &receiveSet, NULL, NULL, wait.getStructure());

        if(selectResult > 0)
        {
            //************************  Packet arrived   *****************

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
                    this->log += "Received packet is not IPv4. Continue receiving...\n";
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
                    this->log += "Received less bytes than minimum header length. Continue receiving...\n";
                }
                continue;
            }

            uint16_t receivedIPtotalLength = ntohs(ip->ip_len);
            uint16_t receivedIPheaderLength = ((uint16_t) ip->ip_hl) * (uint16_t) 4;
            unsigned short payloadLength = (unsigned short) (receivedIPtotalLength - receivedIPheaderLength);

            if(receivedBytes < receivedIPtotalLength)
            {
                if(verbose)
                {
                    this->log += "Received less bytes than announced in header. Continue receiving...\n";
                }
                continue;
            }

            // Computes the IP header checksum to make sure that the IP header is intact.
            // Despite IP header checksum is 2 bytes long, we don't need to apply htons() or ntohs().
            uint16_t tmpChecksum = ip->ip_sum;
            
            // Temporarily sets ip_sum to zero in order to compute checksum. This value will be restored later
            ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
            if(DirectProber::calculateInternetChecksum((uint16_t*) buffer, receivedIPheaderLength) != tmpChecksum)
            {
                if(verbose)
                {
                    this->log += "Error while re-computing IP header checksum. Continue receiving...\n";
                }
                continue;
            }
            ip->ip_sum = tmpChecksum;

            uint8_t IPProtocol = ip->ip_p;
            if(IPProtocol != IPPROTO_ICMP && IPProtocol != IPPROTO_TCP)
            {
                if(verbose)
                {
                    this->log += "Received packet is neither ICMP neither TCP. Continue receiving...\n";
                }
                continue;
            }

            if(IPProtocol == IPPROTO_ICMP)
            {
                /**
                 * Now we are sure that we got an ICMP packet of IPv4. Lets check the content of 
                 * the packet to see if this packet is a reply to the packet we sent out.
                 */
                
                if(verbose)
                {
                    this->log += "Received an ICMP reply.\n";
                }

                // Resets icmp for the arrived packet
                struct icmphdr *icmp = (struct icmphdr*) (buffer + (ip->ip_hl) * 4);

                // First checks that the ICMP checksum of the received packet is correct.
                // Despite ICMP checksum is 2 bytes long, we don't need to apply htons() or ntohs().
                tmpChecksum = icmp->checksum;
                icmp->checksum = 0x0; // Before computing checksum, the sum field must be zero
                
                // We don't need to convert icmp checksum to network-byte-order
                if(DirectProber::calculateInternetChecksum((uint16_t*) icmp, receivedIPtotalLength - receivedIPheaderLength) != tmpChecksum)
                {
                    if(verbose)
                    {
                        this->log += "Error while re-computing ICMP header checksum. Continue receiving...\n";
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

                    // IP identifier of the piggybacked packet is checked especially for multiplexing usingFixedFlowID packets
                    if(payloadip->ip_p == IPPROTO_TCP && ntohs(payloadip->ip_id) == IPIdentifier_16 
                       && ntohs(payloadtcp->source) == srcPort_16 && ntohs(payloadtcp->dest) == dstPort_16)
                    {
                        InetAddress rplyAddress((unsigned long int) ntohl((ip->ip_src).s_addr));
                        
                        ProbeRecord *newRecord = buildProbeRecord(REQTime, 
                                                                  dst, 
                                                                  rplyAddress, 
                                                                  TTL, 
                                                                  ip->ip_ttl, 
                                                                  icmp->type, 
                                                                  icmp->code, 
                                                                  IPIdentifier, 
                                                                  ntohs(ip->ip_id), 
                                                                  payloadip->ip_ttl, 
                                                                  payloadLength, 
                                                                  1, 
                                                                  usingFixedFlowID);
                        
                        if(verbose)
                        {
                            this->log += newRecord->toString();
                        }
                        
                        this->nbSuccessfulProbes++;
                        return newRecord;
                    }
                }
                else
                {
                    if(verbose)
                    {
                        this->log += "An unknown packet has been received. Continue receiving...\n";
                    }
                }
            }
            else if(IPProtocol == IPPROTO_TCP)
            {
                /**
                 * Now we are sure that we got a TCP packet of IPv4. Let us check the content of 
                 * the packet to see if this packet is a reply to the packet we sent out.
                 */
                
                if(verbose)
                {
                    this->log += "Received a TCP reply... ";
                }

                // Reset ICMP for the arrived packet
                struct tcphdr *tcp = (struct tcphdr*) (buffer + (ip->ip_hl) * 4);
                
                if(ntohs(tcp->dest) == srcPort_16 && ntohs(tcp->source) == dstPort_16)
                {
                    this->log += "coming from the right target!\n";
                
                    // This is the reply of the packet that we sent
                    InetAddress rplyAddress((unsigned long int) ntohl((ip->ip_src).s_addr));
                    
                    ProbeRecord *newRecord = buildProbeRecord(REQTime, 
                                                              dst, 
                                                              rplyAddress, 
                                                              TTL, 
                                                              ip->ip_ttl, 
                                                              DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE, 
                                                              DirectProber::PSEUDO_TCP_RESET_ICMP_CODE, 
                                                              IPIdentifier, 
                                                              ntohs(ip->ip_id), 
                                                              0, 
                                                              payloadLength, 
                                                              1, 
                                                              usingFixedFlowID);
                    
                    if(verbose)
                    {
                        this->log += newRecord->toString();
                    }
                    
                    this->nbSuccessfulProbes++;
                    return newRecord;
                }
                else if(verbose)
                {
                    this->log += "not coming from the target. Continue receiving...\n";
                }
            }
        }
        //****************   Select timeout occured   *********************
        else if(selectResult == 0)
        {
            if(verbose)
            {
                this->log += "\nThe select() function timed out. Stopped listening.\n";
            }
            return buildProbeRecord(REQTime, dst, InetAddress(0), TTL, 0, 255, 255, IPIdentifier, 0, 0, 0, 1, usingFixedFlowID);
        }
        //****************   Select error occured    *******************
        else
        {
            if(verbose)
            {
                string errorMsg = "\nThe select() function returned an error: ";
                if(errno == EINVAL)
                    errorMsg += "EINVAL.";
                else if(errno == EINTR)
                    errorMsg += "EINTR.";
                else if(errno == EBADF)
                    errorMsg += "EBADF.";
                else
                    errorMsg += string(strerror(errno)) + ".";
                errorMsg += "Stopped listening.";
                
                this->log += errorMsg + "\n";
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
                                               unsigned short srcIPidentifier, 
                                               unsigned short rplyIPidentifier, 
                                               unsigned char payloadTTL, 
                                               unsigned short payloadLength, 
                                               int probingCost, 
                                               bool usingFixedFlowID)
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
    recordPtr->setSrcIPidentifier(srcIPidentifier);
    recordPtr->setRplyIPidentifier(rplyIPidentifier);
    recordPtr->setPayloadTTL(payloadTTL);
    recordPtr->setPayloadLength(payloadLength);
    recordPtr->setProbingCost(probingCost);
    recordPtr->setUsingFixedFlowID(usingFixedFlowID);
    return recordPtr;
}
