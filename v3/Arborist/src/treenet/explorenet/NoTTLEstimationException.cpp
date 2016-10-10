/*
 * NoTTLEstimationException.cpp
 *
 *  Created on: Sep 17, 2016
 *      Author: grailet
 */

#include "NoTTLEstimationException.h"

NoTTLEstimationException::NoTTLEstimationException(const InetAddress &dst, const string &msg):
NTmapException(msg),
destinationIP(dst)
{

}

NoTTLEstimationException::~NoTTLEstimationException() throw() {}
