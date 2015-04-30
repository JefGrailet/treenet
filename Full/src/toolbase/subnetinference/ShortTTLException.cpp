/*
 * ShortTTLException.cpp
 *
 *  Created on: Oct 2, 2010
 *      Author: root
 */

#include "ShortTTLException.h"

ShortTTLException::ShortTTLException(const InetAddress &dst, const string &msg):
NTmapException(msg),
destinationIP(dst)
{

}

ShortTTLException::~ShortTTLException() throw() {}

