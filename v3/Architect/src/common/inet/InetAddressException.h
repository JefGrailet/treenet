/*
 * InetAddressException.h
 *
 *  Created on: Jul 5, 2008
 *      Author: root
 */

#ifndef INETADDRESSEXCEPTION_H_
#define INETADDRESSEXCEPTION_H_

#include "../exception/NTmapException.h"


class InetAddressException : public NTmapException{
public:
	InetAddressException(const string & msg="IP Address Error Occured...");
	virtual ~InetAddressException() throw();
};

#endif /* INETADDRESSEXCEPTION_H_ */
