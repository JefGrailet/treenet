/*
 * StopException.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: jefgrailet
 *
 * Implements the class described in StopException.H. See this file for more details.
 */

#include "StopException.h"

StopException::StopException(const string &msg)
:runtime_error(msg)
{
}

StopException::~StopException() throw()
{
}

