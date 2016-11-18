/*
 * ShortTTLException.h
 *
 *  Created on: Oct 2, 2010
 *      Author: root
 */

#ifndef SHORTTTLEXCEPTION_H_
#define SHORTTTLEXCEPTION_H_

#include "../../../common/exception/NTmapException.h"
#include "../../../common/inet/InetAddress.h"
#include <string>
using std::string;

class ShortTTLException: public NTmapException
{
public:
    ShortTTLException(const InetAddress &destinationIP, const string &msg = "Not long enough TTL for exploring the site");
    virtual ~ShortTTLException() throw();
    InetAddress & getDestinationIP() { return destinationIP; }
    void setDestinationIP(InetAddress &destinationIP) { this->destinationIP = destinationIP; }
private:
    InetAddress destinationIP;
};

#endif /* SHORTTTLEXCEPTION_H_ */
