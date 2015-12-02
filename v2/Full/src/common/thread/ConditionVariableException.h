/*
 * ConditionVariableException.h
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#ifndef CONDITIONVARIABLEEXCEPTION_H_
#define CONDITIONVARIABLEEXCEPTION_H_

#include <string>
using std::string;

#include "../exception/NTmapException.h"

class ConditionVariableException: public NTmapException {
public:
	ConditionVariableException(const string & msg="Condition Variable Error Occurred");
	virtual ~ConditionVariableException() throw();
};

#endif /* CONDITIONVARIABLEEXCEPTION_H_ */
