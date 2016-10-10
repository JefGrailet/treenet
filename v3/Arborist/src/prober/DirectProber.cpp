/*
 * DirectProber.cpp
 *
 *  Created on: Jul 19, 2008
 *      Author: root
 *
 * Coding style slightly edited by Jean-François Grailet in December 2014.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <iostream>
using std::cout;
using std::endl;
#include <set>
using std::set;
#include <sys/time.h> // For gettimeofday (by J.-F. G)

#include "DirectProber.h"
#include "../common/thread/Thread.h"

const unsigned short DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID = 30000;
const unsigned short DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID = 64000;
const unsigned short DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ = 30000;
const unsigned short DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ = 64000;

const uint16_t DirectProber::MINIMUM_IP_HEADER_LENGTH = 20;
const uint16_t DirectProber::DEFAULT_ICMP_HEADER_LENGTH = 8;
const uint16_t DirectProber::DEFAULT_ICMP_RADOM_DATA_LENGTH = 8;
const uint16_t DirectProber::MINIMUM_UDP_HEADER_LENGTH = 8;
const uint16_t DirectProber::DEFAULT_UDP_RANDOM_DATA_LENGTH = 8;
const uint16_t DirectProber::MINIMUM_TCP_HEADER_LENGTH = 20;
const uint16_t DirectProber::DEFAULT_TCP_RANDOM_DATA_LENGTH = 8;
const uint8_t DirectProber::DEFAULT_IP_VERSION = 4;
const uint8_t DirectProber::DEFAULT_IP_TOS = 0x0;
const uint16_t DirectProber::DEFAULT_IP_FRAGMENT_OFFSET = 0x0;

const uint16_t DirectProber::MAX_UINT16_T_NUMBER = ~(0x00);

const TimeVal DirectProber::DEFAULT_PROBE_REGULATOR_PAUSE_PERIOD(0, 0);
const TimeVal DirectProber::DEFAULT_TIMEOUT_PERIOD(1, 500000);

const int DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT = 6;

const unsigned char DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE = 101;
const unsigned char DirectProber::PSEUDO_TCP_RESET_ICMP_CODE = 101;

const unsigned char DirectProber::ICMP_TYPE_ECHO_REQUEST = 8;
const unsigned char DirectProber::ICMP_TYPE_ECHO_REPLY = 0;
const unsigned char DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE = 3;
const unsigned char DirectProber::ICMP_TYPE_TIME_EXCEEDED = 11;

const unsigned char DirectProber::ICMP_TYPE_TS_REQUEST = 13;
const unsigned char DirectProber::ICMP_TYPE_TS_REPLY = 14;
const uint16_t DirectProber::ICMP_TS_FIELDS_LENGTH = 12;
const size_t DirectProber::TIMESTAMP_LENGTH_BYTES = 4;

const unsigned char DirectProber::ICMP_CODE_NETWORK_UNREACHABLE = 0;
const unsigned char DirectProber::ICMP_CODE_HOST_UNREACHABLE = 1;
const unsigned char DirectProber::ICMP_CODE_PROTOCOL_UNREACHABLE = 2;
const unsigned char DirectProber::ICMP_CODE_PORT_UNREACHABLE = 3;
const unsigned char DirectProber::ICMP_CODE_NETWORK_UNKNOWN = 6;
const unsigned char DirectProber::ICMP_CODE_HOST_UNKNOWN = 7;

const int DirectProber::ICMP_PROTOCOL = IPPROTO_ICMP;
const int DirectProber::TCP_PROTOCOL = IPPROTO_TCP;
const int DirectProber::UDP_PROTOCOL = IPPROTO_UDP;

const int DirectProber::CONJECTURED_GLOBAL_INTERNET_DIAMETER = 48;

DirectProber::DirectProber(string &attentionMessage, 
                           int proto, 
                           int tcpUdpRoundRobinSocketCount, 
                           const TimeVal &timeoutSeconds, 
                           const TimeVal &prp, 
                           unsigned short lowBoundSrcPortICMPid, 
                           unsigned short upBoundSrcPortICMPid, 
                           unsigned short lowBoundDstPortICMPseq, 
                           unsigned short upBoundDstPortICMPseq, 
                           bool v) throw(SocketException):
sendSocketRAW(-1),
icmpReceiveSocketRAW(-1),
tcpudpReceiveSocketCount(0),
tcpudpReceiveSockets(0),
tcpudpReceivePorts(0),
activeTCPUDPReceiveSocketIndex(0),
probingProtocol(proto),
timeout(timeoutSeconds),
probeRegulatingPausePeriod(prp),
lastProbeTime(0, 0),
lowerBoundSrcPortICMPid(lowBoundSrcPortICMPid),
upperBoundSrcPortICMPid(upBoundSrcPortICMPid),
lowerBoundDstPortICMPseq(lowBoundDstPortICMPseq),
upperBoundDstPortICMPseq(upBoundDstPortICMPseq),
probeCountStatistic(0),
verbose(v), 
log("")
{
    this->setAttentionMsg(attentionMessage);

    // Ensures that the protocol is ICMP, UDP, or TCP
    if(probingProtocol != IPPROTO_ICMP && probingProtocol != IPPROTO_UDP && probingProtocol != IPPROTO_TCP)
    {
        throw SocketException("Unsupported protocol. DirectProber only supports IPPROTO_ICMP, IPPROTO_UDP, and IPPROTO_TCP.");
    }
    // Ensures the lower and upper bounds of TCP - UDP ports or ICMP id/sequence are correct
    if(lowBoundSrcPortICMPid >= upBoundSrcPortICMPid)
    {
        if(probingProtocol == IPPROTO_ICMP)
            throw SocketException("Lower bound ICMP identifier is not less than the upper bound ICMP identifier.");
        else if(probingProtocol == IPPROTO_UDP)
            throw SocketException("Lower bound UDP source port is not less than the upper bound UDP source port.");
        else
            throw SocketException("Lower bound TCP source port is not less than the upper bound TCP source port.");
    }
    if(lowBoundDstPortICMPseq >= upBoundDstPortICMPseq)
    {
        if(probingProtocol == IPPROTO_ICMP)
            throw SocketException("Lower bound ICMP sequence is not less than the upper bound ICMP sequence.");
        else if(probingProtocol == IPPROTO_UDP)
            throw SocketException("Lower bound UDP destination port is not less than the upper bound UDP destination port.");
        else
            throw SocketException("Lower bound TCP destination port is not less than the upper bound TCP destination port.");
    }

    /*
     * Ensures the socket fds won't be 0, 1 or 2 
     *
     * Note by J.-F. Grailet: former code did not close files, resulting in running out of file
     * descriptors after a while, crashing the program.
     */

    char devnull[] = "/dev/null";
    
    int fopen1 = open(devnull, O_RDONLY);
    int fopen2 = open(devnull, O_RDONLY);
    int fopen3 = open(devnull, O_RDONLY);

    if (fopen1 < 0 || fopen2 < 0 || fopen3 < 0)
    {
        if(fopen1 >= 0)
            close(fopen1);
        if(fopen2 >= 0)
            close(fopen2);
        if(fopen3 >= 0)
            close(fopen3);
        throw SocketException("Can NOT open file descriptor to the NULL device");
    }
    
    close(fopen1);
    close(fopen2);
    close(fopen3);
    
    if(verbose)
    {
        this->log += "\n";
    }

    // Creates sending socket
    if((sendSocketRAW = socket(PF_INET, SOCK_RAW, probingProtocol)) == -1)
    {
        if(errno == EACCES)
            throw SocketException("Can NOT create sending socket. The process does not have appropriate privileges.");
        // We do not elaborate other error types
        else
            throw SocketException("Can NOT create sending socket.");
    }
    else
    {
        if(verbose)
        {
            stringstream ss;
            ss << "Sending raw socket has been created, socket identifier is ";
            ss << sendSocketRAW << ".\n";
            this->log += ss.str();
        }
    }
    
    // Tells the kernel that packets sent through this socket has IP level header fields already set
    const int onSend = 1;
    if (setsockopt(sendSocketRAW, IPPROTO_IP, IP_HDRINCL, &onSend, sizeof(onSend)) < 0)
    {
        throw SocketException("Can NOT set sending socket IP_HDRINCL");
    }

    // Creates receiving socket for ICMP (the protocol is always IPPROTO_ICMP, because we are collecting ICMP messages)
    if((icmpReceiveSocketRAW = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        if(errno == EACCES)
            throw SocketException("Can NOT create receiving ICMP raw socket. The process does not have appropriate privileges.");
        // We do not elaborate other error types
        else
            throw SocketException("Can NOT create receiving ICMP raw socket.");
    }
    else
    {
        if(fcntl(icmpReceiveSocketRAW, F_SETFL, O_NONBLOCK) == -1)
            throw SocketException("Can NOT set receiving ICMP raw socket into non-blocking mode");
        else
        {
            if(verbose)
            {
                stringstream ss;
                ss << "ICMP receiving raw socket has been created, socket identifier is ";
                ss << icmpReceiveSocketRAW << ".\n";
                this->log += ss.str();
            }
        }
    }

    const int onRecv = 1;
    if (setsockopt(icmpReceiveSocketRAW, IPPROTO_IP, IP_HDRINCL, &onRecv, sizeof(onRecv)) < 0)
    {
        throw SocketException("Can NOT set receiving socket IP_HDRINCL");
    }

    // Creates receive socket for DirectTCPProber to collect TCP RESET packets
    if(probingProtocol == IPPROTO_TCP || probingProtocol == IPPROTO_UDP)
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        int bindAttempt = 0;

        tcpudpReceiveSockets = new int[tcpUdpRoundRobinSocketCount];
        tcpudpReceivePorts = new unsigned short int[tcpUdpRoundRobinSocketCount];
        for(int i = 0; i < tcpUdpRoundRobinSocketCount; i++)
        {
            tcpudpReceiveSockets[i] = -1;
            tcpudpReceivePorts[i] = 0;
        }

        tcpudpReceiveSocketCount = 0;
        int i = 0;
        bool stopTCPUDPsocketCreation = false;
        while(!stopTCPUDPsocketCreation && 
              tcpudpReceiveSocketCount < tcpUdpRoundRobinSocketCount &&
              (lowerBoundSrcPortICMPid + i + 1) < upperBoundSrcPortICMPid)
        {
            // Creates receive socket(s)
            if((tcpudpReceiveSockets[tcpudpReceiveSocketCount] = socket(PF_INET, SOCK_RAW, probingProtocol)) == -1)
            {
                if(errno == EACCES)
                    throw SocketException("Can NOT create receiving TCP or UDP raw Socket. The process does not have appropriate privileges.");
                // We do not elaborate other error types
                else
                    throw SocketException("Can NOT create receiving TCP or UDP raw Socket");
            }
            else
            {
                if(fcntl(tcpudpReceiveSockets[tcpudpReceiveSocketCount], F_SETFL, O_NONBLOCK) == -1)
                    throw SocketException("Can NOT set receiving TCP or UDP raw socket into non-blocking mode");
            }
            
            // Binds the socket
            if (setsockopt(tcpudpReceiveSockets[tcpudpReceiveSocketCount], SOL_SOCKET, SO_REUSEADDR, &onRecv, sizeof(onRecv)) < 0)
            {
                throw SocketException("Can NOT set receiving TCP or UDP socket SO_REUSEADDR");
            }

            bindAttempt = 0;
            while((lowerBoundSrcPortICMPid + i + 1) < upperBoundSrcPortICMPid)
            {
                local.sin_port = htons((uint16_t) (lowerBoundSrcPortICMPid + i + 1));
                if (bind(tcpudpReceiveSockets[tcpudpReceiveSocketCount], (struct sockaddr*) &local, sizeof(local)) < 0) 
                {
                    if(bindAttempt >= 256)
                    {
                        // Closes the latest created but unbinded socket
                        close(tcpudpReceiveSockets[tcpudpReceiveSocketCount]);
                        stopTCPUDPsocketCreation = true;
                        break;
                    }
                }
                else
                {
                    tcpudpReceivePorts[i] = (lowerBoundSrcPortICMPid + i + 1);
                    tcpudpReceiveSocketCount++;
                    if(verbose)
                    {
                        stringstream ss;
                        if(probingProtocol == IPPROTO_TCP)
                            ss << "TCP ";
                        else
                            ss << "UDP ";
                        ss << "receiving raw socket has been created on port ";
                        ss << tcpudpReceivePorts[i] << ", socket identifier is ";
                        ss << tcpudpReceiveSockets[i] << ".\n";
                        this->log += ss.str();
                    }
                    break;
                }
                i++;
                bindAttempt++;
            } // End of inner while
            
            // Closes the latest created but unbinded socket
            if((lowerBoundSrcPortICMPid + i + 1) >= upperBoundSrcPortICMPid)
            {
                close(tcpudpReceiveSockets[tcpudpReceiveSocketCount]);
            }
            i++;
        } // End of while
        
        if(tcpudpReceiveSocketCount<1)
        {
            throw SocketException("There must be at least one TCP or UDP receive socket");
        }
    } // probingProtocol==IPPROTO_TCP || probingProtocol==IPPROTO_UDP

    // Initializes random number generator for random data buffer
    srand(time(0));
}

DirectProber::~DirectProber()
{
    FD_ZERO(&(this->receiveSet));
    if(sendSocketRAW >= 0 && close(sendSocketRAW) == -1)
        cout << "[ITOM] Can NOT close the send raw socket" << endl;

    if(icmpReceiveSocketRAW >= 0 && close(icmpReceiveSocketRAW) == -1)
        cout << "[ITOM] Can NOT close the ICMP receive socket" << endl;

    if(tcpudpReceiveSockets != 0)
    {
        for(int i = 0; i < tcpudpReceiveSocketCount; i++)
        {
            if(tcpudpReceiveSockets[i] >= 0 && close(tcpudpReceiveSockets[i]) == -1)
            {
                cout << "[ITOM] Can NOT close the receive TCP raw socket " << i << endl;
            }
        }
        delete[] tcpudpReceiveSockets;
        delete[] tcpudpReceivePorts;
    }
}

unsigned char DirectProber::estimateHopDistanceSingleProbe(const InetAddress &src, 
                                                           const InetAddress &dst, 
                                                           unsigned char middleTTL, 
                                                           bool useFixedFlowID, 
                                                           int *numberOfSentPackets)
{
    ProbeRecord *firstICMPEchoReply = 0;
    ProbeRecord *rec = 0;
    unsigned char TTL = middleTTL;
    const int MAX_CONSECUTIVE_ANONYMOUS_COUNT = 3;
    int consecutiveAnonymousCount = 0;
    std::set<InetAddress> appearingIPset;
    bool stop = false;
    unsigned char hopDistance = 0;
    (*numberOfSentPackets) = 0;
    
    // Forward probing
    do
    {
        rec = singleProbe(src, dst, TTL, useFixedFlowID);
        (*numberOfSentPackets) += rec->getProbingCost();

        if(rec->isAnonymousRecord())
        {
            consecutiveAnonymousCount++;
        }
        else
        {
            if(appearingIPset.find(rec->getRplyAddress()) == appearingIPset.end())
            {
                appearingIPset.insert(rec->getRplyAddress());
            }
            else
            {
                // Loop discovered
                firstICMPEchoReply = 0;
                delete rec;
                rec = 0;
                break;
            }
            consecutiveAnonymousCount = 0;
            if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                firstICMPEchoReply = rec;
                rec = 0;
                break;
            }
            else if(rec->getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                stop = true;
            }
        }
        delete rec;
        rec = 0;
        TTL++;
    }
    while(!stop && consecutiveAnonymousCount<MAX_CONSECUTIVE_ANONYMOUS_COUNT && 
          firstICMPEchoReply == 0 && TTL <= (unsigned char) DirectProber::CONJECTURED_GLOBAL_INTERNET_DIAMETER);

    // Forward probing
    if(TTL == middleTTL && firstICMPEchoReply != 0)
    {
        TTL--;
        while(TTL>1)
        {
            rec = singleProbe(src, dst, TTL, useFixedFlowID);
            (*numberOfSentPackets) += rec->getProbingCost();
            if(!rec->isAnonymousRecord())
            {
                if(rec->getRplyICMPtype()==DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    delete firstICMPEchoReply;
                    firstICMPEchoReply = rec;
                    rec = 0;
                }
                else
                {
                    delete rec;
                    rec = 0;
                    break;
                }
            }
            else
            {
                delete rec;
                rec = 0;
                break;
            }
            TTL--;
        }
    }

    if(firstICMPEchoReply == 0)
        delete rec;
    else
    {
        hopDistance = firstICMPEchoReply->getReqTTL();
        delete firstICMPEchoReply;
    }

    return hopDistance;

}

unsigned char DirectProber::estimateHopDistanceDoubleProbe(const InetAddress &src, 
                                                           const InetAddress &dst, 
                                                           unsigned char middleTTL, 
                                                           bool useFixedFlowID, 
                                                           int *numberOfSentPackets)
{
    ProbeRecord *firstICMPEchoReply = 0;
    ProbeRecord *rec = 0;
    unsigned char TTL = middleTTL;
    const int MAX_CONSECUTIVE_ANONYMOUS_COUNT = 3;
    int consecutiveAnonymousCount = 0;
    std::set<InetAddress> appearingIPset;
    bool stop = false;
    unsigned char hopDistance = 0;
    (*numberOfSentPackets) = 0;
    
    // Forward probing
    do
    {
        rec = doubleProbe(src, dst, TTL, useFixedFlowID);
        (*numberOfSentPackets) += rec->getProbingCost();

        if(rec->isAnonymousRecord())
        {
            consecutiveAnonymousCount++;
        }
        else
        {
            if(appearingIPset.find(rec->getRplyAddress()) == appearingIPset.end())
            {
                appearingIPset.insert(rec->getRplyAddress());
            }
            else
            {
                // Loop discovered
                firstICMPEchoReply=0;
                delete rec;
                rec = 0;
                break;
            }
            consecutiveAnonymousCount = 0;
            if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                firstICMPEchoReply = rec;
                rec = 0;
                break;
            }
            else if(rec->getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                stop = true;
            }
        }
        delete rec;
        rec = 0;
        TTL++;
    }
    while(!stop && consecutiveAnonymousCount < MAX_CONSECUTIVE_ANONYMOUS_COUNT && 
          firstICMPEchoReply == 0 && TTL <= (unsigned char) DirectProber::CONJECTURED_GLOBAL_INTERNET_DIAMETER);

    // Forward probing
    if(TTL == middleTTL && firstICMPEchoReply != 0)
    {
        TTL--;
        while(TTL > 1)
        {
            rec = doubleProbe(src, dst, TTL, useFixedFlowID);
            (*numberOfSentPackets) += rec->getProbingCost();
            if(!rec->isAnonymousRecord())
            {
                if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    delete firstICMPEchoReply;
                    firstICMPEchoReply = rec;
                    rec = 0;
                }
                else
                {
                    delete rec;
                    rec = 0;
                    break;
                }
            }
            else
            {
                delete rec;
                rec = 0;
                break;
            }
            TTL--;
        }
    }

    if(firstICMPEchoReply == 0)
    {
        delete rec;
    }
    else
    {
        hopDistance = firstICMPEchoReply->getReqTTL();
        delete firstICMPEchoReply;
    }

    return hopDistance;
}

void DirectProber::RESET_SELECT_SET()
{
    FD_ZERO(&receiveSet);
    if(probingProtocol == IPPROTO_TCP)
    {
        FD_SET(tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex], &receiveSet);
        // cout << "fd_set tcp: " << tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex] << "    ";
    }
    FD_SET(icmpReceiveSocketRAW, &receiveSet); // Always listen on ICMP socket
    // cout << "fd_set icmp: " << icmpReceiveSocketRAW << endl;
}

int DirectProber::GET_READY_SOCKET_DESCRIPTOR()
{
    // J.-F. Grailet: removed old debug messages here because they added nothing useful.

    if(FD_ISSET(icmpReceiveSocketRAW, &receiveSet) != 0)
    {
        return icmpReceiveSocketRAW;
    }
    if(probingProtocol == IPPROTO_TCP)
    {
        if(FD_ISSET(tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex], &receiveSet) != 0)
        {
            return tcpudpReceiveSockets[activeTCPUDPReceiveSocketIndex];
        }
    }
    return -1;
}

void DirectProber::fillRandomDataBuffer()
{
    uint8_t randomChar;
    for(int i = 0; i < DEFAULT_RANDOM_DATA_BUFFER_LENGH; i++)
    {
        randomChar = rand() % 256;
        randomDataBuffer[i] = randomChar;
    }
}

void DirectProber::regulateProbingFrequency()
{
    if(probeRegulatingPausePeriod.isPositive())
    {
        TimeVal period = *(TimeVal::getCurrentSystemTime()) - lastProbeTime;
        if(period.compare(probeRegulatingPausePeriod) == -1)
        {
            Thread::invokeSleep(probeRegulatingPausePeriod - period);
        }
    }
}

unsigned short DirectProber::getAvailableSrcPortICMPid(bool useFixedFlowID)
{
    if(probingProtocol == IPPROTO_TCP || probingProtocol == IPPROTO_UDP)
    {
        if(useFixedFlowID == true)
            activeTCPUDPReceiveSocketIndex = 0;
        
        return tcpudpReceivePorts[activeTCPUDPReceiveSocketIndex];
    }
    else
    {
        return lowerBoundSrcPortICMPid + (rand() % (upperBoundSrcPortICMPid - lowerBoundSrcPortICMPid));
    }
}

unsigned short DirectProber::getAvailableDstPortICMPseq(bool useFixedFlowID)
{
    if(probingProtocol == IPPROTO_TCP || probingProtocol == IPPROTO_UDP)
    {
        if(useFixedFlowID == true)
            return lowerBoundDstPortICMPseq + ((upperBoundDstPortICMPseq - lowerBoundDstPortICMPseq) / 2); // Just use the middle destination port
        else
            return lowerBoundDstPortICMPseq + (rand() % (upperBoundDstPortICMPseq - lowerBoundDstPortICMPseq));
    }
    else
    {
        return lowerBoundDstPortICMPseq + (rand() % (upperBoundDstPortICMPseq - lowerBoundDstPortICMPseq));
    }
}

int DirectProber::getNextActiveTCPUDPreceiveSocketIndex()
{
    return ((activeTCPUDPReceiveSocketIndex + 1) % tcpudpReceiveSocketCount);
}

int DirectProber::getPreviousActiveTCPUDPreceiveSocketIndex()
{
    return ((activeTCPUDPReceiveSocketIndex - 1) % tcpudpReceiveSocketCount);
}

/**
 * I got this implementation from http://ubuntuforums.org/showthread.php?t=1098805
 * ********************IMPORTANT****************************
 * Even though IP checksum or ICMP checksum is 2 bytes long we don't need to
 * convert it from htons() or ntohs(). So just plug this value as it is.
 */

uint16_t DirectProber::onesComplementAddition(uint16_t *buff, unsigned int nbytes)
{
    if(buff == 0 || nbytes == 0)
    {
        return 0;
    }

    uint32_t sum = 0;
    for (; nbytes > 1; nbytes -= 2)
    {
        sum += *buff++;
    }

    if (nbytes == 1)
    {
        sum += *(uint8_t*) buff;
    }

    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return sum;
}

uint16_t DirectProber::onesComplementAddition(uint16_t num1, uint16_t num2)
{
    uint32_t sum = 0;
    sum = num1 + num2;
    
    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return sum;
}

uint32_t DirectProber::getUTTimeSinceMidnight()
{
    timeval UTTime;
    gettimeofday(&UTTime, NULL);
    
    unsigned long inMilliseconds = (UTTime.tv_sec % 86400) * 1000 + (UTTime.tv_usec / 1000);
    
    return (uint32_t) inMilliseconds;
}

string DirectProber::getAndClearLog()
{
    string tmp = this->log;
    this->log = "";
    return tmp;
}
