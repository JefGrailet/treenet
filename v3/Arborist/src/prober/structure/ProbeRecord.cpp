/*
 * ProbeRecord.cpp
 *
 *  Created on: ?
 *      Author: root
 *
 * Implements what is described in ProbeRecord.h. See that file for more details on the purpose 
 * of this class.
 */

#include "ProbeRecord.h"

ProbeRecord::ProbeRecord(InetAddress dstAddr, 
                         InetAddress rpAddr, 
                         TimeVal rqTime, 
                         TimeVal rpTime, 
                         unsigned char rqTTL, 
                         unsigned char rpTTL, 
                         unsigned char rpICMPtype, 
                         unsigned char rpICMPcode, 
                         unsigned short srIPidentifier, 
                         unsigned short rpIPidentifier, 
                         unsigned char payTTL, 
                         unsigned short payLen, 
                         unsigned long oTs, 
                         unsigned long rTs, 
                         unsigned long tTs, 
                         int prbCost, 
                         bool ffID):
dstAddress(dstAddr), 
rplyAddress(rpAddr), 
reqTime(rqTime), 
rplyTime(rpTime), 
reqTTL(rqTTL), 
rplyTTL(rpTTL), 
rplyICMPtype(rpICMPtype), 
rplyICMPcode(rpICMPcode), 
srcIPidentifier(srIPidentifier), 
rplyIPidentifier(rpIPidentifier), 
payloadTTL(payTTL), 
payloadLength(payLen), 
originateTs(oTs), 
receiveTs(rTs), 
transmitTs(tTs), 
probingCost(prbCost), 
usingFixedFlowID(ffID)
{

}

ProbeRecord::ProbeRecord(const ProbeRecord &toClone):
dstAddress(toClone.dstAddress), 
rplyAddress(toClone.rplyAddress), 
reqTime(toClone.reqTime), 
rplyTime(toClone.rplyTime), 
reqTTL(toClone.reqTTL), 
rplyTTL(toClone.rplyTTL), 
rplyICMPtype(toClone.rplyICMPtype), 
rplyICMPcode(toClone.rplyICMPcode), 
srcIPidentifier(toClone.srcIPidentifier), 
rplyIPidentifier(toClone.rplyIPidentifier), 
payloadTTL(toClone.payloadTTL), 
payloadLength(toClone.payloadLength), 
originateTs(toClone.originateTs), 
receiveTs(toClone.receiveTs), 
transmitTs(toClone.transmitTs), 
probingCost(toClone.probingCost), 
usingFixedFlowID(toClone.usingFixedFlowID)
{

}

ProbeRecord::~ProbeRecord()
{

}

const ProbeRecord &ProbeRecord::operator=(const ProbeRecord &right)
{
    if(this != &right)
    {
        this->dstAddress = right.dstAddress;
        this->rplyAddress = right.rplyAddress;
        this->reqTime = right.reqTime;
        this->rplyTime = right.rplyTime;
        this->reqTTL = right.reqTTL;
        this->rplyTTL = right.rplyTTL;
        this->rplyICMPtype = right.rplyICMPtype;
        this->rplyICMPcode = right.rplyICMPcode;
        this->srcIPidentifier = right.srcIPidentifier;
        this->rplyIPidentifier = right.rplyIPidentifier;
        this->payloadTTL = right.payloadTTL;
        this->payloadLength = right.payloadLength;
        this->originateTs = right.originateTs;
        this->receiveTs = right.receiveTs;
        this->transmitTs = right.transmitTs;
        this->probingCost = right.probingCost;
        this->usingFixedFlowID = right.usingFixedFlowID;
    }
    return *this;
}

string ProbeRecord::toString()
{
    stringstream logStream;
    
    logStream << "\nProbe record:\n";
    logStream << "Request TTL: " << (int) reqTTL << "\n";
    logStream << "Request IP identifier: " << srcIPidentifier << "\n";
    logStream << "Request destination address: " << dstAddress << "\n";
    logStream << "Reply address: " << rplyAddress << "\n";
    logStream << "Reply TTL: " << (int) rplyTTL << "\n";
    logStream << "Reply IP identifier: " << rplyIPidentifier << "\n";
    logStream << "Reply ICMP type: " << (int) rplyICMPtype << " - ";
    switch((int) rplyICMPtype)
    {
        case 0:
            logStream << "Echo reply";
            break;
        case 3:
            logStream << "Destination unreachable\n";
            logStream << "Reply ICMP code: " << (int) rplyICMPcode << " - ";
            switch((int) rplyICMPcode)
            {
                case 0: logStream << "Net Unreachable"; break;
                case 1: logStream << "Host Unreachable"; break;
                case 2: logStream << "Protocol Unreachable"; break;
                case 3: logStream << "Port Unreachable"; break;
                case 4: logStream << "Fragmentation Needed and Don't Fragment was Set"; break;
                case 5: logStream << "Source Route Failed"; break;
                case 6: logStream << "Destination Network Unknown"; break;
                case 7: logStream << "Destination Host Unknown"; break;
                case 8: logStream << "Source Host Isolated"; break;
                case 9: logStream << "Communication with Destination Network is Administratively Prohibited"; break;
                case 10: logStream << "Communication with Destination Host is Administratively Prohibited"; break;
                case 11: logStream << "Destination Network Unreachable for Type of Service"; break;
                case 12: logStream << "Destination Host Unreachable for Type of Service"; break;
                case 13: logStream << "Communication Administratively Prohibited"; break;
                case 14: logStream << "Host Precedence Violation"; break;
                case 15: logStream << "Precedence cutoff in effect"; break;
                default: logStream << "Unknown"; break;
            }
            break;
        case 4:
            logStream << "Source quench";
            break;
        case 5:
            logStream << "Redirect\n";
            logStream << "Reply ICMP code: " << (int) rplyICMPcode << " - ";
            switch((int) rplyICMPcode)
            {
                case 0: logStream << "Redirect Datagram for the Network (or subnet)"; break;
                case 1: logStream << "Redirect Datagram for the Host"; break;
                case 2: logStream << "Redirect Datagram for the Type of Service and Network"; break;
                case 3: logStream << "Redirect Datagram for the Type of Service and Host"; break;
                default: logStream << "Unknown"; break;
            }
            break;
        case 6:
            logStream << "Alternate host address";
            break;
        case 8:
            logStream << "Echo";
            break;
        case 11:
            logStream << "Time exceeded\n";
            logStream << "Reply ICMP code: " << (int) rplyICMPcode << " - ";
            switch((int) rplyICMPcode)
            {
                case 0: logStream << "Time to Live exceeded in Transit"; break;
                case 1: logStream << "Fragment Reassembly Time Exceeded"; break;
                default: logStream << "Unknown"; break;
            }
            break;
        case 12:
            logStream << "Parameter problem\n";
            logStream << "Reply ICMP code: " << (int) rplyICMPcode << " - ";
            switch((int) rplyICMPcode)
            {
                case 0: logStream << "Pointer indicates the error"; break;
                case 1: logStream << "Missing a Required Option"; break;
                case 2: logStream << "Bad Length"; break;
                default: logStream << "Unknown"; break;
            }
            break;
        case 13:
            logStream << "Timestamp";
            break;
        case 14:
            logStream << "Timestamp reply";
            break;
        case 101:
            logStream << "TCP Reset";
            break;
        case 255:
            logStream << "Unset (dummy record for unsuccessful probe)";
            break;
        default:
            logStream << "Unexpected type, see documentation and check code\n";
            logStream << "Reply ICMP code: " << (int) rplyICMPcode;
            break;
        
        /*
         * Note: there are many possible other ICMP types, but they should occur rarely while 
         * using TreeNET (plus, some of the types above are themselves unlikely). Therefore, 
         * they are not documented further in the log. See a reference on ICMP for more 
         * details.
         */
    }
    logStream << "\n";
    logStream << "Payload TTL: " << (int) payloadTTL << "\n";
    logStream << "Payload length: " << payloadLength << "\n";
    logStream << "Request time: " << reqTime << "\n";
    logStream << "Reply time: " << rplyTime << "\n";
    
    if(rplyICMPtype == 14) // ICMP timestamp reply
    {
        logStream << "\nTimestamp fields:\n";
        logStream << "Originate TS: " << originateTs << "\n";
        logStream << "Receive TS: " << receiveTs << "\n";
        logStream << "Transmit TS: " << transmitTs << "\n";
    }
    
    return logStream.str();
}

bool ProbeRecord::isATimeout()
{
    if(rplyAddress == InetAddress(0) && (int) rplyICMPtype == 255 && (int) rplyICMPcode == 255 && (int) rplyTTL == 0)
        return true;
    return false;
}
