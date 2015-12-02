/*
 * NTmapException.cpp
 *
 *  Created on: Jul 26, 2012
 *      Author: engin
 */

#include "NTmapException.h"

NTmapException::NTmapException(const string & msg)
:runtime_error(msg)
{
}

NTmapException::~NTmapException()throw()
{
}

