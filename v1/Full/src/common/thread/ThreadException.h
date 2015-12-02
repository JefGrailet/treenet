/*
 * ThreadException.h
 *
 *  Created on: Jul 8, 2008
 *      Author: root
 */

#ifndef THREADEXCEPTION_H_
#define THREADEXCEPTION_H_

#include <string>
using std::string;

#include "../exception/NTmapException.h"

class ThreadException: public NTmapException {
public:
	ThreadException(const string & msg="Thread Error Occurred");
	virtual ~ThreadException() throw();
};

#endif /* THREADEXCEPTION_H_ */
