/*
 * SocketReceiveException.h
 *
 *  Created on: Jul 8, 2008
 *      Author: root
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#ifndef SOCKETRECEIVEEXCEPTION_H_
#define SOCKETRECEIVEEXCEPTION_H_

#include "SocketException.h"

class SocketReceiveException: public SocketException
{
public:
	SocketReceiveException(const string &msg = "Can NOT receive packet");
	virtual ~SocketReceiveException() throw();
};

#endif /* SOCKETRECEIVEEXCEPTION_H_ */
