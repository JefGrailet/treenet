/*
 * UnresponsiveIPException.cpp
 *
 *  Created on: Sep 30, 2010
 *      Author: engin
 */

#include "UnresponsiveIPException.h"

UnresponsiveIPException::UnresponsiveIPException(const InetAddress &dst, const string &msg):
NTmapException(msg),
destinationIP(dst)
{

}

UnresponsiveIPException::~UnresponsiveIPException() throw() {}

