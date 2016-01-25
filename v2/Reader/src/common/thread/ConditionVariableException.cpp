/*
 * ConditionVariableException.cpp
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#include "ConditionVariableException.h"

ConditionVariableException::ConditionVariableException(const string & msg):
	NTmapException(msg)
{
}

ConditionVariableException::~ConditionVariableException() throw(){

}
