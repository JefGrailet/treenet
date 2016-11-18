/*
 * This file implements the class described in DirectICMPPProber.h. Date at which it was implemented 
 * is unknown (though it should be the same as DirectICMPProber.h).
 */

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <iostream>
using std::cout;
using std::endl;

#include "DirectICMPProber.h"
#include "../../common/thread/Thread.h"

const unsigned short DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER = 0;
const unsigned short DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER = ~0;
const unsigned short DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE = 0;
const unsigned short DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE = ~0;

DirectICMPProber::DirectICMPProber(string &attentionMessage,
                                   const TimeVal &timeoutPeriod,
                                   const TimeVal &probeRegulatorPausePeriod,
                                   unsigned short lowerBoundICMPid,
                                   unsigned short upperBoundICMPid,
                                   unsigned short lowerBoundICMPseq,
                                   unsigned short upperBoundICMPseq,
                                   bool verbose) throw(SocketException):
DirectProber(attentionMessage, 
             IPPROTO_ICMP, 
             DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
             timeoutPeriod, 
             probeRegulatorPausePeriod, 
             lowerBoundICMPid, 
             upperBoundICMPid, 
             lowerBoundICMPseq, 
             upperBoundICMPseq, 
             verbose)
{
    this->usingTimestampRequests = false;
}

DirectICMPProber::~DirectICMPProber()
{
}

ProbeRecord *DirectICMPProber::basic_probe(const InetAddress &src,
                                           const InetAddress &dst,
                                           unsigned short IPIdentifier, 
                                           unsigned char TTL, 
                                           bool usingFixedFlowID, 
                                           unsigned short ICMPidentifier, 
                                           unsigned short ICMPsequence) throw (SocketSendException, SocketReceiveException)
{
    bool timestampRequest = this->usingTimestampRequests;
    uint32_t src_32 = (uint32_t) (src.getULongAddress());
    uint32_t dst_32 = (uint32_t) (dst.getULongAddress());
    uint16_t IPIdentifier_16 = (uint16_t) IPIdentifier;
    uint8_t TTL_8 = (uint8_t) TTL;
    uint16_t ICMPidentifier_16 = (uint16_t) ICMPidentifier;
    uint16_t ICMPsequence_16 = (uint16_t) ICMPsequence;
    
    // Exception stating usingFixedFlowID and ICMP timestamp request are not compatible
    if(timestampRequest && usingFixedFlowID)
    {
        string msg = "It is not possible to both use the ICMP timestamp request message and have ";
        msg + "a fixed flow ID (with Paris traceroute). Please only use one feature at a time.";
        throw SocketSendException(msg);
    }

    // 1) Prepares packet to send; sets IP fields
    uint32_t IPHeaderLength = DirectProber::MINIMUM_IP_HEADER_LENGTH;
    struct ip *ip = (struct ip*) buffer;
    ip->ip_v = DirectProber::DEFAULT_IP_VERSION;
    ip->ip_hl = IPHeaderLength / ((uint32_t) 4); // In terms of 4 byte units
    ip->ip_tos = DirectProber::DEFAULT_IP_TOS;
    uint16_t totalPacketLength = IPHeaderLength + DirectProber::DEFAULT_ICMP_HEADER_LENGTH;
    // Size depends on the fact that we use timestamp request or not
    if(timestampRequest)
        totalPacketLength += DirectProber::ICMP_TS_FIELDS_LENGTH;
    else
        totalPacketLength += DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH + (uint32_t) getAttentionMsg().length();
    ip->ip_len = htons(totalPacketLength);
    ip->ip_id = htons(IPIdentifier_16);
    ip->ip_off = htons(DirectProber::DEFAULT_IP_FRAGMENT_OFFSET);
    ip->ip_ttl = TTL_8;
    ip->ip_p = IPPROTO_ICMP;
    (ip->ip_src).s_addr = htonl(src_32);
    (ip->ip_dst).s_addr = htonl(dst_32);
    ip->ip_sum = 0x0; // Before computing checksum, the sum field must be zero
    
    // Even though IP checksum is 2 bytes long we dont need to apply htons() or ntohs() on this field
    ip->ip_sum = DirectProber::calculateInternetChecksum((uint16_t*) buffer, IPHeaderLength);

    // Set ICMP fields
    struct icmphdr *icmp = (struct icmphdr*) (buffer + IPHeaderLength);

    // Building up an ICMP timestamp request
    uint32_t UTTimeSinceMidnight = 0; // For saving in probeRecord later
    if(timestampRequest)
    {
        // Main fields
        icmp->type = DirectProber::ICMP_TYPE_TS_REQUEST;
        icmp->code = 0x0;
        (icmp->un).echo.id = htons(ICMPidentifier_16);
        (icmp->un).echo.sequence = htons(ICMPsequence_16);
        
        UTTimeSinceMidnight = DirectProber::getUTTimeSinceMidnight();
        
        // Writing Originate (= cur UT time since midnight), Receive and Transmit (= 0) timestamps
        uint8_t *icmpts = (((uint8_t*) icmp) + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
        memcpy(icmpts, &UTTimeSinceMidnight, DirectProber::TIMESTAMP_LENGTH_BYTES);
        icmpts += DirectProber::TIMESTAMP_LENGTH_BYTES;
        memset(icmpts, 0x0, DirectProber::TIMESTAMP_LENGTH_BYTES);
        icmpts += DirectProber::TIMESTAMP_LENGTH_BYTES;
        memset(icmpts, 0x0, DirectProber::TIMESTAMP_LENGTH_BYTES);
        
        // Checksum
        icmp->checksum = 0x0; // Before computing checksum, the sum field must be zero
        icmp->checksum = DirectProber::calculateInternetChecksum((uint16_t*) icmp, 
        DirectProber::DEFAULT_ICMP_HEADER_LENGTH + DirectProber::ICMP_TS_FIELDS_LENGTH);
        
        /*
         * Even though ICMP checksum is 2 bytes long we don't need to apply htons() or ntohs() on 
         * this field.
         */
    }
    else
    {
        // Main fields
        icmp->type = DirectProber::ICMP_TYPE_ECHO_REQUEST;
        icmp->code = 0x0;
        (icmp->un).echo.id = htons(ICMPidentifier_16);
        (icmp->un).echo.sequence = htons(ICMPsequence_16);

        /**
         * The random data must have been generated by the calling function
         * to make DOS attack suspections less.
         */
    
        fillRandomDataBuffer();
        uint8_t *icmpdata=(((uint8_t*) icmp) + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
        if(usingFixedFlowID)
        {
            memset(icmpdata, 0x0, DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH);
        
            /*
             * Only uses six random bytes, the other two bytes will be computed later to make ones 
             * complement sum all-ones.
             */
            
            memcpy(icmpdata, randomDataBuffer, DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH - 2);
        }
        else
        {
            memcpy(icmpdata, randomDataBuffer, DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH);
        }

        icmpdata += DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH;
    
        // Copying attention message, setting checksum to 0 before computing it
        memcpy(icmpdata, getAttentionMsg().c_str(), getAttentionMsg().length());
        icmpdata += getAttentionMsg().length();
        icmp->checksum = 0x0;
        
        /*
         * Even though ICMP checksum is 2 bytes long we don't need to apply htons() or ntohs() on 
         * this field.
         */
        
        if(usingFixedFlowID)
        {
            // Uses middle icmpseq as the constant checksum value
            icmp->checksum = (uint16_t) ((getLowerBoundDstPortICMPseq() + getUpperBoundDstPortICMPseq()) / 2); 
            uint16_t ocsum = DirectProber::onesComplementAddition((uint16_t*) icmp, 
            DirectProber::DEFAULT_ICMP_HEADER_LENGTH + DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH + getAttentionMsg().length());
        
            // Takes the difference between all-ones and sum
            uint16_t ocdiff = DirectProber::onesComplementSubtraction(DirectProber::MAX_UINT16_T_NUMBER, ocsum);
            memcpy(((uint8_t*) icmp) + DirectProber::DEFAULT_ICMP_HEADER_LENGTH + DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH - 2, 
            (uint8_t *) (&ocdiff), 2);
        }
        else
        {
            // Regular checksum
            icmp->checksum = DirectProber::calculateInternetChecksum((uint16_t*) icmp, 
            DirectProber::DEFAULT_ICMP_HEADER_LENGTH + DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH + getAttentionMsg().length());
        }
    }

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
        logStream << ", RSID = " << this->icmpReceiveSocketRAW << "]\n";
    
        // Actual details on the probe.
        logStream << "ICMP probing:\n";
        logStream << "Source address: " << src << "\n";
        logStream << "Destination address: " << dst << "\n";
        logStream << "IP identifier (source): " << IPIdentifier << "\n";
        logStream << "Initial TTL: " << (int) TTL << "\n";
        logStream << "ICMP identifier: " << ICMPidentifier << "\n";
        logStream << "ICMP sequence: " << ICMPsequence << "\n";

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
            throw SocketSendException("Can NOT send the Echo Request packet");
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

        if(selectResult > 0)
        {
            // ************************  packet arrived   *****************

            // First makes sure that the packet arrived to the socket we are interested in
            int readySocketDescriptor = GET_READY_SOCKET_DESCRIPTOR();
            if(readySocketDescriptor < 0)
            {
                continue;
            }

            /**
             * recvfrom method reads an entire packet into the buffer for SOCK_RAW,SOCK_DGRAM, and SOCK_SEQPACKET types of sockets,
             * if the buffer is less than the packet length the rest of the packet is overflowing part is discarded.
             * However, for SOCK_STREAM it reads in the portions of a packet as soon as it becomes available.
             *
             * Since our socket is SOCK_RAW, we can freely assume that the packet is received as a whole without worrying about
             * packet portions received. However, though our socket receives only ICMP packets we must make sure that the
             * version of the received packet is IPv4 and the received packet is a response to the packet that we sent.
             * discarded
             */

            receivedBytes = recvfrom(readySocketDescriptor, // Socket handle
                                     this->buffer, // Receives buffer to store the received packets
                                     DEFAULT_DIRECT_ICMP_PROBER_BUFFER_SIZE, // Receives buffer size in bytes
                                     0, // Flags
                                     (struct sockaddr*) &fromAddress,
                                     &fromAddressLength);

            if(receivedBytes == -1)
            {
                perror("Socket Receive Exception Error Message");
                throw SocketReceiveException("Can NOT receive packets");
            }

            // Resets IP for the arrived packet
            ip = (struct ip*) buffer;

            /**
             * The very first byte received holds the incoming packet's  IP version and IP header length, two 4 bit length fields.
             * If the IP version is not IPv4 or IP header length is less than the minimum header length we ignore this packet and
             * continue receiving for the next packet
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

            /*
             * Computes the IP header checksum to make sure that the IP header is intact. Despite 
             * IP header checksum is 2 bytes long, we don't need to apply htons() or ntohs(). 
             * ip_sum is also temporarily set to zero in order to compute checksum. This value 
             * will be restored later.
             */
            
            uint16_t tmpChecksum = ip->ip_sum;
            ip->ip_sum = 0x0;
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
                    this->log += "Received packet is not an ICMP one. Continue receiving...\n";
                }
                continue;
            }

            /**
             * Now we are sure that we got an ICMP packet of IPv4. Lets check the content of the packet to see if this packet
             * is a reply to the packet we sent out.
             */

            // Resets ICMP for the arrived packet
            icmp = (struct icmphdr*) (buffer + (ip->ip_hl) * 4);

            // First check is the ICMP checksum of the received packet is correct
            // Despite icmp checksum is 2 bytes long we don't need to apply htons() or ntohs()
            tmpChecksum = icmp->checksum;
            icmp->checksum = 0x0; // Before computing checksum, the sum field must be zero

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
                // Sets payload IP and ICMP for time exceeded and unreachable replies.
                struct ip *payloadip = (struct ip*) (buffer + receivedIPheaderLength + DirectProber::DEFAULT_ICMP_HEADER_LENGTH);
                struct icmphdr *payloadicmp = (struct icmphdr*) (buffer + receivedIPheaderLength + 
                DirectProber::DEFAULT_ICMP_HEADER_LENGTH + (payloadip->ip_hl) * 4);

                // This is the reply of the packet that we sent
                if(payloadicmp->type == DirectProber::ICMP_TYPE_ECHO_REQUEST &&
                   ntohs(payloadip->ip_id) == IPIdentifier_16 &&
                   ntohs((payloadicmp->un).echo.id) == ICMPidentifier_16 &&
                   ntohs((payloadicmp->un).echo.sequence) == ICMPsequence_16)
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
                                                              0, 
                                                              0, 
                                                              0, 
                                                              1, 
                                                              usingFixedFlowID);
                     
                    if(verbose)
                    {
                        this->log += newRecord->toString();
                    }
                     
                    return newRecord;
                }
            }
            else if(icmp->type == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                // This is the reply of the packet that we sent
                if(ntohs((icmp->un).echo.id) == ICMPidentifier_16 && ntohs((icmp->un).echo.sequence) == ICMPsequence_16)
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
                                                              0, 
                                                              payloadLength, 
                                                              0, 
                                                              0, 
                                                              0, 
                                                              1, 
                                                              usingFixedFlowID);
                    
                    if(verbose)
                    {
                        this->log += newRecord->toString();
                    }
                    
                    return newRecord;
                }
            }
            // Received an ICMP timestamp reply
            else if(icmp->type == DirectProber::ICMP_TYPE_TS_REPLY)
            {
                // Picks the obtained timestamps
                uint8_t* timestamps = (buffer + (ip->ip_hl) * 4) + 12;
                
                // + 12 because 8 bytes for ICMP headers and 4 bytes for originate timestamp

                unsigned long receiveTs = 0, transmitTs = 0;
                receiveTs = (unsigned short) timestamps[0] * 256 * 256 * 256;
                receiveTs += (unsigned short) timestamps[1] * 256 * 256;
                receiveTs += (unsigned short) timestamps[2] * 256;
                receiveTs += (unsigned short) timestamps[3];
                
                transmitTs = (unsigned short) timestamps[4] * 256 * 256 * 256;
                transmitTs += (unsigned short) timestamps[5] * 256 * 256;
                transmitTs += (unsigned short) timestamps[6] * 256;
                transmitTs += (unsigned short) timestamps[7];

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
                                                          0, 
                                                          payloadLength, 
                                                          (unsigned long) UTTimeSinceMidnight, 
                                                          receiveTs, 
                                                          transmitTs, 
                                                          1, 
                                                          usingFixedFlowID);
                
                if(verbose)
                {
                    this->log += newRecord->toString();
                }
                
                return newRecord;
            }
            else
            {
                if(verbose)
                {
                    this->log += "An unknown packet has been received. Continue receiving...\n";
                }
            }


        }
        else if(selectResult == 0)
        {
            //****************   Select timeout occured   ********************
            if(verbose)
            {
                this->log += "\nThe select() function timed out. Stopped listening.\n";
            }
            return buildProbeRecord(REQTime, dst, InetAddress(0), TTL, 0, 255, 255, IPIdentifier, 0, 0, 0, 0, 0, 0, 1, usingFixedFlowID);
        }
        else
        {
            //**********************     Select error occured    *******************
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


ProbeRecord *DirectICMPProber::buildProbeRecord(const auto_ptr<TimeVal> &reqTime, 
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
    recordPtr->setOriginateTs(originateTs);
    recordPtr->setReceiveTs(receiveTs);
    recordPtr->setTransmitTs(transmitTs);
    recordPtr->setProbingCost(probingCost);
    recordPtr->setUsingFixedFlowID(usingFixedFlowID);
    return recordPtr;
}
