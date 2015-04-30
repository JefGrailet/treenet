/*
 * UndesignatedPivotInterface.h
 *
 *  Created on: Oct 1, 2010
 *      Author: engin
 */

#ifndef UNDESIGNATEDPIVOTINTERFACE_H_
#define UNDESIGNATEDPIVOTINTERFACE_H_

#include "../../common/exception/NTmapException.h"
#include "../../common/inet/InetAddress.h"
#include <string>
using std::string;

class UndesignatedPivotInterface: public NTmapException
{
public:
	UndesignatedPivotInterface(const InetAddress &destinationIP, 
	                           unsigned char destinationIPdistance, 
	                           const string &msg = "Cannot designate a pivot interface for the target site");
	virtual ~UndesignatedPivotInterface() throw();
	InetAddress & getDestinationIP() { return destinationIP; }
	void setDestinationIP(InetAddress &destinationIP) { this->destinationIP = destinationIP; }
	unsigned char getDestinationIPdistance() { return destinationIPdistance; }
	void setDestinationIPdistance(unsigned char destinationIPdistance) { this->destinationIPdistance = destinationIPdistance; }
private:
	InetAddress destinationIP;
	unsigned char destinationIPdistance;
};

#endif /* UNDESIGNATEDPIVOTINTERFACE_H_ */

