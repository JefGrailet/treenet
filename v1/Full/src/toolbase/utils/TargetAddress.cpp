/*
 * TargetAddress.cpp
 *
 *  Created on: Jul 13, 2012
 *      Author: engin
 */

#include "TargetAddress.h"

TargetAddress::TargetAddress(InetAddress dst, unsigned char startHop, unsigned char endHop):
address(dst),
startTTL(startHop),
endTTL(endHop)
{

}

TargetAddress::~TargetAddress() {}
