/*
 * UndesignatedPivotInterface.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: engin
 */

#include "UndesignatedPivotInterface.h"

UndesignatedPivotInterface::UndesignatedPivotInterface(const InetAddress &dst, unsigned char dstHop, const string &msg):
NTmapException(msg),
destinationIP(dst),
destinationIPdistance(dstHop)
{

}

UndesignatedPivotInterface::~UndesignatedPivotInterface() throw() {}

