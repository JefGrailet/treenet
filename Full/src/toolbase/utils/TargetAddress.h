/*
 * TargetAddress.h
 *
 *  Created on: Jul 13, 2012
 *      Author: engin
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#ifndef TARGETADDRESS_H_
#define TARGETADDRESS_H_

#include <iostream>
using std::ostream;

#include "../../common/inet/InetAddress.h"
#include "../ToolBase.h"

class TargetAddress
{
public:

	friend ostream & operator<<(ostream &out, const TargetAddress &ta)
	{
		out << *(ta.address.getHumanReadableRepresentation()) + string(", ") 
		+ StringUtils::Uchar2string(ta.startTTL) + string(", ") + 
		StringUtils::Uchar2string(ta.endTTL);
		return out;
	}
	
	TargetAddress(InetAddress targetAddress = InetAddress(0), 
	              unsigned char startTTL = 1, 
	              unsigned char endTTL = (ToolBase::CONJECTURED_GLOBAL_INTERNET_DIAMETER + 1));
	virtual ~TargetAddress();
	InetAddress address;
	unsigned char startTTL;
	unsigned char endTTL;
};

#endif /* TARGETADDRESS_H_ */
