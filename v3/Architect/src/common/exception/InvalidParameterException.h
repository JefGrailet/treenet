/*
 * InvalidParameterException.h
 *
 *  Created on: Jul 28, 2008
 *      Author: root
 */

#ifndef INVALIDPARAMETEREXCEPTION_H_
#define INVALIDPARAMETEREXCEPTION_H_

#include <string>
using std::string;

#include "NTmapException.h"

class InvalidParameterException: public NTmapException {
public:
	InvalidParameterException(const string & msg="The parameter given is invalid");
	virtual ~InvalidParameterException()throw();
};

#endif /* INVALIDPARAMETEREXCEPTION_H_ */
