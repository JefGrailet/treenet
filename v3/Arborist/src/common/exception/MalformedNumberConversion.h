/*
 * MalformedNumberConversion.h
 *
 *  Created on: Jul 2, 2008
 *      Author: root
 */

#ifndef MALFORMEDNUMBERCONVERSION_H_
#define MALFORMEDNUMBERCONVERSION_H_

#include "NTmapException.h"

class MalformedNumberConversion: public NTmapException {
public:
	MalformedNumberConversion(const string &msg="Can NOT convert");
	virtual ~MalformedNumberConversion()throw();
};

#endif /* MALFORMEDNUMBERCONVERSION_H_ */
