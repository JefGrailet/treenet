/*
 * SubnetBarrierException.h
 *
 *  Created on: Sep 30, 2010
 *      Author: engin
 *
 * Slightly edited by J.-F. Grailet on Sept 12, 2016.
 */

#ifndef SUBNETBARRIEREXCEPTION_H_
#define SUBNETBARRIEREXCEPTION_H_

#include "../../../common/exception/NTmapException.h"

class SubnetBarrierException: public NTmapException
{
public:
    SubnetBarrierException(unsigned char barrierPrefix);
    virtual ~SubnetBarrierException() throw();
    unsigned char barrierPrefix; // This is the last valid prefix that you can grow subnet
    
    // N.B.: there used to be a default value for barrierPrefix, but it has never been used.
};

#endif /* SUBNETBARRIEREXCEPTION_H_ */
