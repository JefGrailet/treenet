/*
 * Undescribed class (though usage is obvious). Edited by J.-F. Grailet to improve coding style.
 */

#ifndef PROBERECORD_H_
#define PROBERECORD_H_

#include <iostream>
using std::ostream;
#include <iomanip>
using std::setw;
using std::setiosflags;
using std::ios;

#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"

class ProbeRecord
{
public:
	friend ostream &operator<<(ostream &out, const ProbeRecord &record)
	{
		if(record.isAnonymousRecord())
		{
			out << "dstAddress:" << setiosflags(ios::left) << setw(15) << record.dstAddress
			<< " rplyAddress:" << setiosflags(ios::left) << setw(30) << "***"
			<< " reqTTL:" << setiosflags(ios::left) << setw(3) << (int) record.reqTTL
			<< " rplyIPidentifier:" << setiosflags(ios::left) << setw(5) << "***"
			<< " rplyICMPtype:" << setiosflags(ios::left) << setw(2) << "***"
			<< " rplyICMPcode:" << setiosflags(ios::left) << setw(2) << "***"
			<< " rplyTTL:" << setiosflags(ios::left) << setw(3) << "***"
			<< " payloadTTL:" << setiosflags(ios::left) << setw(3) << "***"
			<< " probingCost:" << setiosflags(ios::left) << setw(3) << (int) record.probingCost
			<< " usingFixedFlowID:" << setiosflags(ios::left) << setw(7) << (record.usingFixedFlowID == true ? "true" : "false");
			if(record.RR == 0)
			{
				out << " RR[0]:*";
			}
			else
			{
				out << " RR[" << record.RRlength << "]: ";
				for(int i = 0; i < record.RRlength; i++)
				{
					out << record.RR[i] << " ";
				}
			}
			out << " reqTime:" << setiosflags(ios::left) << setw(10) << record.reqTime.getSecondsPart() 
			<< "s / " << setiosflags(ios::left) << setw(10) << record.reqTime.getMicroSecondsPart() << setiosflags(ios::left) << "mics"
			<< " rplyTime:" << setiosflags(ios::left) << setw(10) << record.rplyTime.getSecondsPart() << "s / " << setiosflags(ios::left) 
			<< setw(10) << record.rplyTime.getMicroSecondsPart() << setiosflags(ios::left) << "mics";
		}
		else
		{
			out<< "dstAddress:" << setiosflags(ios::left) << setw(15) << record.dstAddress
			<< " rplyAddress:" << setiosflags(ios::left) << setw(30) << record.rplyAddress
			<< " reqTTL:" << setiosflags(ios::left) << setw(3) << (int) record.reqTTL
			<< " rplyIPidentifier:" << setiosflags(ios::left) << setw(5) << record.rplyIPidentifier
			<< " rplyICMPtype:" << setiosflags(ios::left) << setw(2) << (int) record.rplyICMPtype
			<< " rplyICMPcode:" << setiosflags(ios::left) << setw(2) << (int) record.rplyICMPcode
			<< " rplyTTL:" << setiosflags(ios::left) << setw(3) << (int) record.rplyTTL
			<< " payloadTTL:" << setiosflags(ios::left) << setw(3) << (int) record.payloadTTL
			<< " probingCost:" << setiosflags(ios::left) << setw(3) << (int) record.probingCost
			<< " usingFixedFlowID:" << setiosflags(ios::left) << setw(7) << (record.usingFixedFlowID == true ? "true" : "false");
			if(record.RR == 0)
			{
				out << " RR[0]: ";
			}
			else
			{
				out<< " RR[" <<record.RRlength<< "]: ";
				for(int i = 0; i < record.RRlength; i++)
				{
					out << record.RR[i] << " ";
				}
			}
			out << " reqTime:" << setiosflags(ios::left) << setw(10) << record.reqTime.getSecondsPart() << "s / " 
			<< setiosflags(ios::left) << setw(10) << record.reqTime.getMicroSecondsPart() << setiosflags(ios::left) << "mics"
			<< " rplyTime:" << setiosflags(ios::left) << setw(10) << record.rplyTime.getSecondsPart() << "s / " 
			<< setiosflags(ios::left) << setw(10) << record.rplyTime.getMicroSecondsPart() << setiosflags(ios::left) << "mics";

		}
		return out;
	}
	
	static const int MAXIMUM_RECORDE_ROUTE_SIZE;
	ProbeRecord(InetAddress dstAddress = InetAddress(0),
			    InetAddress rplyAddress = InetAddress(0),
			    TimeVal reqTime = TimeVal(0, 0),
			    TimeVal rplyTime = TimeVal(0, 0),
			    unsigned char reqTTL = 0,
			    unsigned char rplyTTL = 0,
			    unsigned char rplyICMPtype = 255,
			    unsigned char rplyICMPcode = 255,
			    unsigned short rplyIPidentifier = 0,
			    unsigned char payloadTTL = 0,
			    int probingCost = 1,
			    bool usingFixedFlowID = false,
			    InetAddress *RR = 0,
			    int RRlength = 0);
	
	// Copy constructor
	ProbeRecord(const ProbeRecord & toClone);
	virtual ~ProbeRecord();
	const ProbeRecord &operator=(const ProbeRecord &right); // Assignment operator
	
	// Query methods
	bool isAnonymousRecord() const { return rplyAddress.isUnset(); }

	// Setters
	void setDstAddress(const InetAddress &addr) { (this->dstAddress).setInetAddress(addr.getULongAddress()); }
	void setRplyAddress(const InetAddress &addr) { (this->rplyAddress).setInetAddress(addr.getULongAddress()); }
	void setReqTime(TimeVal &reqTime) { this->reqTime = reqTime; }
	void setRplyTime(TimeVal &rplyTime) { this->rplyTime = rplyTime; }
	void setReqTTL(unsigned char reqTTL) { this->reqTTL = reqTTL; }
	void setRplyTTL(unsigned char rplyTTL) { this->rplyTTL = rplyTTL; }
	void setRplyICMPtype(unsigned char rplyICMPtype) { this->rplyICMPtype = rplyICMPtype; }
	void setRplyICMPcode(unsigned char rplyICMPcode) { this->rplyICMPcode = rplyICMPcode; }
	void setRplyIPidentifier(unsigned short rplyIPidentifier) { this->rplyIPidentifier = rplyIPidentifier; }
	void setPayloadTTL(unsigned char payloadTTL) { this->payloadTTL = payloadTTL; }
	void setProbingCost(int probingCost) { this->probingCost = probingCost; }
	void setRR(InetAddress *RR) { this->RR = RR; }
	void setRRlength(int RRlength) { this->RRlength = RRlength; }
	void setUsingFixedFlowID(bool usingFixedFlowID) { this->usingFixedFlowID = usingFixedFlowID; }

	// Accessers
	const InetAddress &getDstAddress() const { return dstAddress; }
	const InetAddress &getRplyAddress() const { return rplyAddress; }
	const TimeVal & getReqTime() const { return reqTime; }
	const TimeVal & getRplyTime() const { return rplyTime; }
	unsigned char getReqTTL() const { return reqTTL; }
	unsigned char getRplyTTL() const { return rplyTTL; }
	unsigned char getRplyICMPtype() const { return rplyICMPtype; }
	unsigned char getRplyICMPcode() const { return rplyICMPcode; }
	unsigned short getRplyIPidentifier() const { return rplyIPidentifier; }
	unsigned char getPayloadTTL() const { return payloadTTL; }
	int getProbingCost() const { return this->probingCost; }
	bool getUsingFixedFlowID() const { return this->usingFixedFlowID; }
	
	/**
	 * RR is an array of InetAddress* collected from a path if record route feature is enabled. 
	 * When RR is NULL there is no RR and RRcont is zero.
	 *
	 *
	 * The array is delimited with a NULL pointer. Note that, getRR just returns the pointer to 
	 * the start of the array and if the ProbeRecord object holding the RR array is deleted, the 
	 * array will be deleted as well. As a result, if you need to keep RR around after deleting 
	 * the ProbeRecord object call getRRclone to get a copy along with the ownership of the 
	 * InetAddress* objects as well as the new RR array.
	 */
	
	InetAddress *const getRR() const { return RR; }
	int getRRlength() const { return this->RRlength; }

protected:
	InetAddress dstAddress;
	InetAddress rplyAddress;
	TimeVal reqTime;
	TimeVal rplyTime;
	unsigned char reqTTL;
	unsigned char rplyTTL;
	unsigned char rplyICMPtype; // 101 implies TCP RESET obtained
	unsigned char rplyICMPcode; // 101 implies TCP RESET obtained
	unsigned short rplyIPidentifier;
	unsigned char payloadTTL;
	int probingCost;
	bool usingFixedFlowID;
	InetAddress *RR;
	int RRlength;
};

#endif /* PROBERECORD_H_ */

