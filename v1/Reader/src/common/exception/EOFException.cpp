/*
 * EOFException.cpp
 *
 *  Created on: Jul 16, 2012
 *      Author: engin
 */

#include "EOFException.h"

EOFException::EOFException(const string &msg):
NTmapException(msg)
{

}

EOFException::~EOFException()  throw() {}

