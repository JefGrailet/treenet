/*
 * BadInputException.cpp
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * Implements the class described in BadInputException.H. See this file for more details.
 */

#include "BadInputException.h"

BadInputException::BadInputException(const string &msg)
:runtime_error(msg)
{
}

BadInputException::~BadInputException() throw()
{
}

