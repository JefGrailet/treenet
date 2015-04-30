/*
 * SubnetBarrierException.h
 *
 *  Created on: Sep 30, 2010
 *      Author: engin
 */

#ifndef SUBNETBARRIEREXCEPTION_H_
#define SUBNETBARRIEREXCEPTION_H_

#include "../../common/exception/NTmapException.h"
#include "../ToolBase.h"

class SubnetBarrierException: public NTmapException
{
public:
	SubnetBarrierException(unsigned char barrierPrefix = ToolBase::MIN_CORE_IP_SUBNET_PREFIX);
	virtual ~SubnetBarrierException() throw();
	unsigned char barrierPrefix; // This is the last valid prefix that you can grow subnet
};

#endif /* SUBNETBARRIEREXCEPTION_H_ */

