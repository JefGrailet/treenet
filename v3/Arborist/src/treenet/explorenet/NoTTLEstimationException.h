/*
 * NoTTLEstimationException.h
 *
 *  Created on: Sep 17, 2016
 *      Author: grailet
 */

#ifndef NOTTLESTIMATIONEXCEPTION_H_
#define NOTTLESTIMATIONEXCEPTION_H_

#include "../../common/exception/NTmapException.h"
#include "../../common/inet/InetAddress.h"
#include <string>
using std::string;

class NoTTLEstimationException: public NTmapException
{
public:
    NoTTLEstimationException(const InetAddress &destinationIP, const string &msg = "Unable to reliably evaluate distance (in TTL) to target IP");
    virtual ~NoTTLEstimationException() throw();
    InetAddress & getDestinationIP() { return destinationIP; }
    void setDestinationIP(InetAddress &destinationIP) { this->destinationIP = destinationIP; }
private:
    InetAddress destinationIP;
};

#endif /* NOTTLESTIMATIONEXCEPTION_H_ */
