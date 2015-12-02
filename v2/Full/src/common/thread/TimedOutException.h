/*
 * TimedOutException.h
 *
 *  Created on: Jul 12, 2008
 *      Author: root
 */

#ifndef TIMEDOUTEXCEPTION_H_
#define TIMEDOUTEXCEPTION_H_

#include <string>
using std::string;

#include "../exception/NTmapException.h"

class TimedOutException : public NTmapException{
public:
	TimedOutException(const string & msg="The wait(timeout) method has returned due to timeout");
	virtual ~TimedOutException() throw();
};

#endif /* TIMEDOUTEXCEPTION_H_ */
