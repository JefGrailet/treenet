/*
 * DirectProber.h
 *
 *  Created on: Jul 19, 2008
 *      Author: root
 *
 * Modifications by J.-F. Grailet;
 * -December 2014: coding style slightly edited.
 * -Augustus 2016: removed the loose source and record route options, because they are unused by 
 *  both TreeNET and ExploreNEt v2.1 and because the IETF (see RFC 7126) reports that packets 
 *  featuring these options are widely dropped, and that the default policy of a router receiving 
 *  such packets should be to drop them anyway due to security concerns.
 */

#ifndef DIRECTPROBER_H_
#define DIRECTPROBER_H_

#define DEFAULT_RANDOM_DATA_BUFFER_LENGH 8

#include <string>
using std::string;
#include <iostream>
using std::cout;
using std::endl;
#include <inttypes.h>
#include <cstdlib>

#include "exception/SocketException.h"
#include "../common/inet/InetAddress.h"
#include "./structure/ProbeRecord.h"
#include "exception/SocketSendException.h"
#include "exception/SocketReceiveException.h"
#include "../common/date/TimeVal.h"

class DirectProber
{
public:

    static const unsigned short DEFAULT_LOWER_SRC_PORT_ICMP_ID;
    static const unsigned short DEFAULT_UPPER_SRC_PORT_ICMP_ID;
    static const unsigned short DEFAULT_LOWER_DST_PORT_ICMP_SEQ;
    static const unsigned short DEFAULT_UPPER_DST_PORT_ICMP_SEQ;

    static const uint16_t MINIMUM_IP_HEADER_LENGTH;
    static const uint16_t DEFAULT_ICMP_HEADER_LENGTH;
    static const uint16_t DEFAULT_ICMP_RADOM_DATA_LENGTH;
    static const uint16_t MINIMUM_UDP_HEADER_LENGTH;
    static const uint16_t DEFAULT_UDP_RANDOM_DATA_LENGTH;
    static const uint16_t MINIMUM_TCP_HEADER_LENGTH;
    static const uint16_t DEFAULT_TCP_RANDOM_DATA_LENGTH;
    static const uint8_t DEFAULT_IP_VERSION;
    static const uint8_t DEFAULT_IP_TOS;
    static const uint16_t DEFAULT_IP_FRAGMENT_OFFSET;

    static const uint16_t MAX_UINT16_T_NUMBER;

    static const TimeVal DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD;
    static const TimeVal DEFAULT_TIMEOUT_PERIOD;

    static const int DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;

    static const unsigned char PSEUDO_TCP_RESET_ICMP_TYPE;
    static const unsigned char PSEUDO_TCP_RESET_ICMP_CODE;

    static const unsigned char ICMP_TYPE_ECHO_REQUEST;
    static const unsigned char ICMP_TYPE_ECHO_REPLY;
    static const unsigned char ICMP_TYPE_DESTINATION_UNREACHABLE;
    static const unsigned char ICMP_TYPE_TIME_EXCEEDED;
    
    // Added by J.-F. Grailet
    static const unsigned char ICMP_TYPE_TS_REQUEST;
    static const unsigned char ICMP_TYPE_TS_REPLY;
    static const uint16_t ICMP_TS_FIELDS_LENGTH; // In bytes
    static const size_t TIMESTAMP_LENGTH_BYTES;

    static const unsigned char ICMP_CODE_NETWORK_UNREACHABLE;
    static const unsigned char ICMP_CODE_HOST_UNREACHABLE;
    static const unsigned char ICMP_CODE_PROTOCOL_UNREACHABLE;
    static const unsigned char ICMP_CODE_PORT_UNREACHABLE;
    static const unsigned char ICMP_CODE_NETWORK_UNKNOWN;
    static const unsigned char ICMP_CODE_HOST_UNKNOWN;

    static const int ICMP_PROTOCOL;
    static const int TCP_PROTOCOL;
    static const int UDP_PROTOCOL;

    static const int CONJECTURED_GLOBAL_INTERNET_DIAMETER;

    static uint16_t onesComplementAddition(uint16_t *buff, unsigned int nbytes);
    static uint16_t onesComplementAddition(uint16_t num1, uint16_t num2);
    static uint16_t onesComplementSubtraction(uint16_t num1, uint16_t num2) { return onesComplementAddition(num1, ~num2); } // num1 - num2
    static uint16_t calculateInternetChecksum(uint16_t *buff, unsigned int nbytes) { return ~(onesComplementAddition(buff, nbytes)); }

    DirectProber(string &attentionMessage, 
                 int probingProtocol, 
                 int tcpUdpRoundRobinSocketCount, 
                 const TimeVal &timeoutSeconds, 
                 const TimeVal &probeRegulatingPausePeriod, 
                 unsigned short lowerBoundSrcPortICMPid, 
                 unsigned short upperBoundSrcPortICMPid, 
                 unsigned short lowerBoundDstPortICMPseq, 
                 unsigned short upperBoundDstPortICMPseq, 
                 bool verbose) throw (SocketException);
    virtual ~DirectProber();
    
    // Accessers and setters
    void setAttentionMsg(string &msg)
    {
        // "NOT an ATTACK (mail: Jean-Francois.Grailet@student.ulg.ac.be)"
        this->attentionMessage = msg.substr(0, 64);
    }
    
    string &getAttentionMsg() { return this->attentionMessage; }
    void setTimeout(const TimeVal &timeout) { this->timeout = timeout; }
    const TimeVal &getTimeout() const { return this->timeout; }
    int getProbingProtocol() const { return this->probingProtocol; }
    void setProbeRegulatingPausePeriod(const TimeVal &probeRegulatingPausePeriod)
    {
        if(probeRegulatingPausePeriod.isPositive() || probeRegulatingPausePeriod.isZero())
        {
            this->probeRegulatingPausePeriod=probeRegulatingPausePeriod;
        }
    }

    const TimeVal &getProbeRegulatingPausePeriod() { return this->probeRegulatingPausePeriod; }

    unsigned short getLowerBoundSrcPortICMPid() { return lowerBoundSrcPortICMPid; }
    unsigned short getUpperBoundSrcPortICMPid() { return upperBoundSrcPortICMPid; }
    unsigned short getLowerBoundDstPortICMPseq() { return lowerBoundDstPortICMPseq; }
    unsigned short getUpperBoundDstPortICMPseq() { return upperBoundDstPortICMPseq; }
    unsigned long long getProbeCountStatistic() { return this->probeCountStatistic; }
    bool getVerbose() { return verbose; }
    void setVerbose(bool verbose) { this->verbose = verbose; }

    ProbeRecord *singleProbe(const InetAddress &src, 
                             const InetAddress &dst, 
                             unsigned char TTL, 
                             bool useFixedFlowID) throw (SocketSendException, SocketReceiveException)
    {
        return singleProbe(src, 
                           dst, 
                           rand() % DirectProber::MAX_UINT16_T_NUMBER, 
                           TTL, 
                           useFixedFlowID, 
                           getAvailableSrcPortICMPid(useFixedFlowID), 
                           getAvailableDstPortICMPseq(useFixedFlowID));
    }

    ProbeRecord *doubleProbe(const InetAddress &src, 
                const InetAddress &dst, 
                unsigned char TTL, 
                bool useFixedFlowID) throw (SocketSendException, SocketReceiveException)
    {
        return doubleProbe(src,
                           dst,
                           rand() % DirectProber::MAX_UINT16_T_NUMBER,
                           TTL, 
                           useFixedFlowID, 
                           getAvailableSrcPortICMPid(useFixedFlowID), 
                           getAvailableDstPortICMPseq(useFixedFlowID));
    }

    unsigned char estimateHopDistanceSingleProbe(const InetAddress &src, 
                                                 const InetAddress &dst, 
                                                 unsigned char middleTTL, 
                                                 bool useFixedFlowID, 
                                                 int *numberOfSentPackets);
    unsigned char estimateHopDistanceDoubleProbe(const InetAddress &src, 
                                                 const InetAddress &dst, 
                                                 unsigned char middleTTL, 
                                                 bool useFixedFlowID, 
                                                 int *numberOfSentPackets);
    
    /*
     * Addition by J.-F. Grailet: static method to get UTC time since midnight in milliseconds, 
     * which is necessary for the proper implementation of ICMP timestamp request.
     */
    
    static uint32_t getUTTimeSinceMidnight();
    
    /*
     * Addition by J.-F. Grailet: method to handle the logs, when the verbose mode is activated 
     * (i.e., verbose equals true).
     */
    
    string getAndClearLog();

protected:

    // Prepares and sends a probe packet.
    ProbeRecord *singleProbe(const InetAddress &src, 
            const InetAddress &dst, 
            unsigned short IPIdentifier, 
            unsigned char TTL, 
            bool usingFixedFlowID, 
            unsigned short srcPortORICMPid, 
            unsigned short dstPortORICMPseq) throw (SocketSendException, SocketReceiveException)
    {
        fillRandomDataBuffer();
        ProbeRecord *result = basic_probe(src, 
                                          dst, 
                                          IPIdentifier, 
                                          TTL, 
                                          usingFixedFlowID, 
                                          srcPortORICMPid, 
                                          dstPortORICMPseq);
        result->setProbingCost(1);
        probeCountStatistic++;
        if(probingProtocol == IPPROTO_UDP || probingProtocol == IPPROTO_TCP)
        {
            activeTCPUDPReceiveSocketIndex = getNextActiveTCPUDPreceiveSocketIndex();
        }
        return result;
    }
    
    // Does the same but re-probes the target if the first probe is unsuccessful.
    ProbeRecord *doubleProbe(const InetAddress &src, 
                             const InetAddress &dst, 
                             unsigned short IPIdentifier, 
                             unsigned char TTL, 
                             bool usingFixedFlowID, 
                             unsigned short srcPortORICMPid, 
                             unsigned short dstPortORICMPseq) throw (SocketSendException, SocketReceiveException)
    {
        fillRandomDataBuffer();
        ProbeRecord *result = basic_probe(src, 
                                          dst, 
                                          IPIdentifier, 
                                          TTL,  
                                          usingFixedFlowID, 
                                          srcPortORICMPid, 
                                          dstPortORICMPseq);
        result->setProbingCost(1);
        probeCountStatistic++;
        if(result->isAnonymousRecord())
        {
            delete result;
            result = basic_probe(src, 
                                 dst, 
                                 IPIdentifier, 
                                 TTL, 
                                 usingFixedFlowID, 
                                 srcPortORICMPid, 
                                 dstPortORICMPseq);
            result->setProbingCost(2);
            probeCountStatistic++;
        }
        if(probingProtocol == IPPROTO_UDP || probingProtocol == IPPROTO_TCP)
        {
            activeTCPUDPReceiveSocketIndex = getNextActiveTCPUDPreceiveSocketIndex();
        }
        return result;
    }

    virtual ProbeRecord *basic_probe(const InetAddress &src,
            const InetAddress &dst, 
            unsigned short IPIdentifier, 
            unsigned char TTL, 
            bool usingFixedFlowID, 
            unsigned short srcPortORICMPid, 
            unsigned short dstPortORICMPseq) throw (SocketSendException, SocketReceiveException) = 0;

    void RESET_SELECT_SET();
    int GET_READY_SOCKET_DESCRIPTOR();
    void fillRandomDataBuffer();
    unsigned short getAvailableSrcPortICMPid(bool useFixedFlowID);
    unsigned short getAvailableDstPortICMPseq(bool useFixedFlowID);

    /**
     * This method causes the thread to sleep before probing the next destination. The aim is regulating
     * the release of probe messages with time spaces in order not to cause complaints from ISPs. A flood
     * of probing may be regarded as a Denial of Service Attack by ISPs. Hence the sub classes of
     * this class must call this methof just before probing the destination. The method gets the current time
     * and subtracts from lastProbeTime, if the difference is less than probeRegulatorPeriod the thread
     * sleeps the amount of current time minus probeRegulatorPeriod.
     */
    
    void regulateProbingFrequency();
    
    /**
     * This method must be called by subclasses just after sending a probe message.
     */
    
    inline void updateLastProbingTime() { this->lastProbeTime = *(TimeVal::getCurrentSystemTime()); }
    inline int getHighestSocketIdentifierForSelect()
    {
        // cout << "icmp desc: " << icmpReceiveSocketRAW << "     tcp udp desc: " << tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex] << endl;
        if(tcpudpReceiveSockets == 0)
        {
            return icmpReceiveSocketRAW;
        }
        else
        {
            if(icmpReceiveSocketRAW>tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex])
            {
                return icmpReceiveSocketRAW;
            }
            else
            {
                return tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex];
            }
        }
    }

    int getNextActiveTCPUDPreceiveSocketIndex();
    int getPreviousActiveTCPUDPreceiveSocketIndex();

    string attentionMessage;
    int sendSocketRAW;// Sends ICMP, TCP or UDP packets
    int icmpReceiveSocketRAW; // Receives ICMP packets
    int tcpudpReceiveSocketCount;
    int *tcpudpReceiveSockets;
    unsigned short *tcpudpReceivePorts;
    int activeTCPUDPReceiveSocketIndex;
    int probingProtocol;
    TimeVal timeout; // Timeout period before returning from waiting the reply
    fd_set receiveSet;
    uint8_t randomDataBuffer[DEFAULT_RANDOM_DATA_BUFFER_LENGH];
    TimeVal probeRegulatingPausePeriod;
    TimeVal lastProbeTime;
    
    // N.B.: "SrcPortICMPid" means "source port OR ICMP Id"
    
    const unsigned short lowerBoundSrcPortICMPid;
    const unsigned short upperBoundSrcPortICMPid;
    const unsigned short lowerBoundDstPortICMPseq;
    const unsigned short upperBoundDstPortICMPseq;
    unsigned long long probeCountStatistic;
    
    // Values to handle verbose/debug mode, via small logs displayed from time to time.
    
    bool verbose;
    string log;
};

#endif /* DIRECTPROBER_H_ */
