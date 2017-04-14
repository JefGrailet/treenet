/*
 * This file implements the class described in DirectUDPProber.h. Date at which it was implemented 
 * is unknown (though it should be the same as DirectUDPProber.h). See DirectUDPProber.h for more 
 * details on the goals of this class along the modifications brought to it since ExploreNET v2.1.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <cerrno>

#include <iostream>
using std::cout;
using std::endl;

#include "DirectUDPProber.h"
#include "../../common/thread/Thread.h"

const unsigned short DirectUDPProber::DEFAULT_LOWER_UDP_SRC_PORT = 39000;
const unsigned short DirectUDPProber::DEFAULT_UPPER_UDP_SRC_PORT = 64000;
const unsigned short DirectUDPProber::DEFAULT_LOWER_UDP_DST_PORT = 39000;
const unsigned short DirectUDPProber::DEFAULT_UPPER_UDP_DST_PORT = 64000;

DirectUDPProber::DirectUDPProber(string &attentionMessage, 
                                 int tcpUdpRoundRobinSocketCount, 
                                 const TimeVal &timeoutPeriod, 
                                 const TimeVal &probeRegulatorPausePeriod, 
                                 unsigned short lowerBoundUDPsrcPort, 
                                 unsigned short upperBoundUDPsrcPort, 
                                 unsigned short lowerBoundUDPdstPort, 
                                 unsigned short upperBoundUDPdstPort, 
                                 bool verbose) throw(SocketException):
DirectProber(attentionMessage, 
             IPPROTO_UDP, 
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

DirectUDPProber::~DirectUDPProber()
{
}

ProbeRecord *DirectUDPProber::basic_probe(const InetAddress &src, 
                                          const InetAddress &dst, 
                                          unsigned short IPIdentifier, 
                                          unsigned char TTL, 
                                          bool usingFixedFlowID, 
                                          unsigned short srcPort, 
                                          unsigned short dstPort) throw(SocketSendException, SocketReceiveException)
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
    uint16_t totalPacketLength = IPHeaderLength + DirectProber::MINIMUM_UDP_HEADER_LENGTH 
    + DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH + (uint32_t) getAttentionMsg().length();
    ip->ip_len = htons(totalPacketLength);
    ip->ip_id = htons(IPIdentifier_16);
    ip->ip_off = htons(DirectProber::DEFAULT_IP_FRAGMENT_OFFSET);
    ip->ip_ttl = TTL_8;
    ip->ip_p = IPPROTO_UDP;
    (ip->ip_src).s_addr = htonl(src_32);
    (ip->ip_dst).s_addr = htonl(dst_32);

    ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
    // Even though IP checksum is 2 bytes long we don't need to apply htons() or ntohs() on this field
    ip->ip_sum = DirectProber::calculateInternetChecksum((uint16_t*) buffer, IPHeaderLength);

    // Sets UDP fields
    struct udphdr *udp = (struct udphdr*) (buffer + IPHeaderLength);

    udp->source = htons(srcPort_16);
    udp->dest = htons(dstPort_16);
    // The length must be udp_header length + data length.
    udp->len = htons(DirectProber::MINIMUM_UDP_HEADER_LENGTH + DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH + getAttentionMsg().length());
    udp->check = 0x0;

    /**
     * Set the 8 bytes data portion of the UDP packet in order to keep a single flow
     * and avoid routers applying path balancing. The random data must be generated
     * and put into the randomDataBuffer by the calling function.
     */
    
    fillRandomDataBuffer();
    uint8_t *udpdata = ((uint8_t*) udp + DirectProber::MINIMUM_UDP_HEADER_LENGTH);
    memcpy(udpdata, this->randomDataBuffer, DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH);
    udpdata += DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH;
    memcpy(udpdata, getAttentionMsg().c_str(), getAttentionMsg().length());
    udpdata += getAttentionMsg().length();

    // UDP checksum is calculated over pseudo header
    uint8_t *pseudo = pseudoBuffer;
    memcpy(pseudo, &((ip->ip_src).s_addr), 4);
    pseudo += 4;
    memcpy(pseudo, &((ip->ip_dst).s_addr), 4);
    pseudo += 4;
    memset(pseudo++, 0, 1); // 1 byte padding
    memset(pseudo++, IPPROTO_UDP, 1); // 1 byte protocol
    memcpy(pseudo, &(udp->len), 2);
    pseudo += 2;
    
    // Now original 8 bytes of UDP header plus the data is also part of pseudo header
    memcpy(pseudo, 
           (uint8_t*) udp, 
           DirectProber::MINIMUM_UDP_HEADER_LENGTH + DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH + getAttentionMsg().length());
    
    pseudo += DirectProber::MINIMUM_UDP_HEADER_LENGTH + DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH + getAttentionMsg().length();

    int pseudoBufferLength = pseudo - pseudoBuffer;

    udp->check = DirectProber::calculateInternetChecksum((uint16_t*) pseudoBuffer, pseudoBufferLength);

    // 2) Sends the request packet
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
        logStream << "UDP probing:\n";
        logStream << "Source address: " << src << "\n";
        logStream << "Destination address: " << dst << "\n";
        logStream << "IP identifier (source): " << IPIdentifier << "\n";
        logStream << "Initial TTL: " << (int) TTL << "\n";
        logStream << "Source port: " << srcPort_16 << "\n";
        logStream << "Destination port: " << dstPort_16 << "\n";

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
            throw SocketSendException("Can NOT send the UDP packet");
        }

        totalBytesSent += bytesSent;

    }
    while(totalBytesSent < totalPacketLength);
    auto_ptr<TimeVal> REQTime = TimeVal::getCurrentSystemTime();
    updateLastProbingTime();

    // 3) Receives the reply packet
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
        // cout << "--------------------Wait " << (wait.isPositive() ? "POSITIVE " : "NEGATIVE or ZERO ") << wait << endl;
        if(wait.isUndefined())
        {
            wait.resetToZero();
        }
        RESET_SELECT_SET();
        int selectResult = select(getHighestSocketIdentifierForSelect() + 1, &receiveSet, NULL, NULL, wait.getStructure());

        // Packet arrived
        if(selectResult > 0)
        {
            // First makes sure that the packet arrived to the socket we are interested in
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

            receivedBytes = recvfrom(readySocketDescriptor, // Socket handle
                                     this->buffer, // Receive buffer to store the received packets
                                     DEFAULT_DIRECT_UDP_PROBER_BUFFER_SIZE, // Receive buffer size in bytes
                                     0, // Flags
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
             * We know that the packets of SOCK_RAW received ad whole in contrast to SOCK_STREAM. 
             * However, we do a check here and if it is not correct the packet would be dropped 
             * because we set totalReceivedBytes = 0; and call select() and receive methods again.
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
            uint16_t receivedIPheaderLength = ((uint16_t)ip->ip_hl) * (uint16_t) 4;
            unsigned short payloadLength = (unsigned short) (receivedIPtotalLength - receivedIPheaderLength);

            if(receivedBytes < receivedIPtotalLength)
            {
                if(verbose)
                {
                    this->log += "Received less bytes than announced in header. Continue receiving...\n";
                }
                continue;
            }

            // Computes the IP header checksum to make sure that the IP header is intact
            // Despite IP header checksum is 2 bytes long we don't need to apply htons() or ntohs()
            uint16_t tmpChecksum = ip->ip_sum;
            // Temporarily sets ip_sum to 0 in order to compute checksum. This value will be restored later
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
            if(IPProtocol != IPPROTO_ICMP)
            {
                if(verbose)
                {
                    this->log += "Received packet is neither ICMP neither TCP. Continue receiving...\n";
                }
                continue;
            }

            /**
             * Now we are sure that we got an ICMP packet of IPv4. Lets check the content of the packet to see if this packet
             * is a reply to the packet we sent out.
             */

            // Resets icmp for the arrived packet
            struct icmphdr *icmp = (struct icmphdr*) (buffer + (ip->ip_hl) * 4);

            // First check that the ICMP checksum of the received packet is correct
            // Despite ICMP checksum is 2 bytes long we don't need to apply htons() or ntohs()
            tmpChecksum = icmp->checksum;
            icmp->checksum = 0x0; // Before computing checksum, the sum field must be zero
            if(DirectProber::calculateInternetChecksum((uint16_t*) icmp, receivedIPtotalLength-receivedIPheaderLength) != tmpChecksum)
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
                // Sets payload IP and ICMP for time exceeded and unreachable replies.
                struct ip *payloadip = (struct ip*) (buffer + receivedIPheaderLength + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
                struct udphdr *payloadudp = (struct udphdr*) (buffer + receivedIPheaderLength 
                + DirectProber::MINIMUM_UDP_HEADER_LENGTH + (payloadip->ip_hl) * 4);

                // IP identifier of the piggybacked packet is checked especially for multiplexing usingFixedFlowID packets
                if(payloadip->ip_p == IPPROTO_UDP 
                   && ntohs(payloadip->ip_id) == IPIdentifier_16 
                   && ntohs(payloadudp->source) == srcPort_16 
                   && ntohs(payloadudp->dest) == dstPort_16)
                {
                    // This is the reply of the packet that we sent
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
        // Select timeout occured
        else if(selectResult == 0)
        {
            if(verbose)
            {
                this->log += "\nThe select() function timed out. Stopped listening.\n";
            }
            return buildProbeRecord(REQTime, dst, InetAddress(0), TTL, 0, 255, 255, IPIdentifier, 0, 0, 0, 1, usingFixedFlowID);
        }
        // Select error occured
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
            // To make the compiler happy
            return 0;
        }
    } // End of while(1){
}

ProbeRecord *DirectUDPProber::buildProbeRecord(const auto_ptr<TimeVal> &reqTime, 
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
