

#include "ProbeRecord.h"

const int ProbeRecord::MAXIMUM_RECORDE_ROUTE_SIZE = 9;

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
	                     int prbCost, 
	                     bool ffID, 
	                     InetAddress *RecRec, 
	                     int RRlen):
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
probingCost(prbCost), 
usingFixedFlowID(ffID), 
RR(RecRec), 
RRlength(RRlen)
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
probingCost(toClone.probingCost), 
usingFixedFlowID(toClone.usingFixedFlowID), 
RR(0), 
RRlength(toClone.RRlength)
{
	if(toClone.RR == 0)
	{
		this->RR = 0;
	}
	else
	{
		RR = new InetAddress[toClone.RRlength];
		for(int i = 0; i <toClone.RRlength; i++)
		{
			RR[i] = toClone.RR[i];
		}
	}

}

ProbeRecord::~ProbeRecord()
{
	if(RR != 0)
	{
		delete[] RR;
	}
}

const ProbeRecord &ProbeRecord::operator=(const ProbeRecord &right)
{
	if(this != &right)
	{
		// Releases the memory of RR
		if(this->RR != 0)
		{
			delete[] RR;
			this->RRlength = 0;
		}
		
		// Now reassigns the elements
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
		this->probingCost = right.probingCost;
		this->usingFixedFlowID = right.usingFixedFlowID;
		if(right.RR!=0)
		{
			this->RRlength = right.RRlength;
			this->RR = new InetAddress[right.RRlength];
			for(int i = 0; i < right.RRlength; i++)
			{
				this->RR[i].setInetAddress(right.RR[i].getULongAddress());
			}
		}

	}
	return *this;
}

