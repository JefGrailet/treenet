/*
 * InvalidSubnetException.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
 *
 * Implements the class defined in NetworkTreeNode.h (see this file to learn further about the 
 * goals of such class).
 */

#include "InvalidSubnetException.h"

InvalidSubnetException::InvalidSubnetException(const string &ss, const string &msg):
NTmapException(msg),
subnetString(ss)
{

}

InvalidSubnetException::~InvalidSubnetException() throw() {}
