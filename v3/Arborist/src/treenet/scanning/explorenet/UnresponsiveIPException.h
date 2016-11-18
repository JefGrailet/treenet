/*
 * UnresponsiveIPException.h
 *
 *  Created on: Sep 30, 2010
 *      Author: engin
 */

#ifndef UNRESPONSIVEIPEXCEPTION_H_
#define UNRESPONSIVEIPEXCEPTION_H_

#include "../../../common/exception/NTmapException.h"
#include "../../../common/inet/InetAddress.h"
#include <string>
using std::string;

class UnresponsiveIPException: public NTmapException
{
public:
    UnresponsiveIPException(const InetAddress &destinationIP, const string &msg = "Probing the IP address did not return any response");
    virtual ~UnresponsiveIPException() throw();
    InetAddress & getDestinationIP() { return destinationIP; }
    void setDestinationIP(InetAddress &destinationIP) { this->destinationIP = destinationIP; }
private:
    InetAddress destinationIP;
};

#endif /* UNRESPONSIVEIPEXCEPTION_H_ */
