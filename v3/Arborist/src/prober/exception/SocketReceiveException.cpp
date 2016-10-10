/*
 * SocketReceiveException.cpp
 *
 *  Created on: Jul 8, 2008
 *      Author: root
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#include "SocketReceiveException.h"

SocketReceiveException::SocketReceiveException(const string &msg):SocketException(msg)
{
}

SocketReceiveException::~SocketReceiveException() throw()
{
	// TODO Auto-generated destructor stub
}
