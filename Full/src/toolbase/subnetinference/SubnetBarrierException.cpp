/*
 * SubnetBarrierException.cpp
 *
 *  Created on: Sep 30, 2010
 *      Author: engin
 */

#include "SubnetBarrierException.h"

SubnetBarrierException::SubnetBarrierException(unsigned char bp):
NTmapException(),
barrierPrefix(bp)
{

}

SubnetBarrierException::~SubnetBarrierException() throw() {}

