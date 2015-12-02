/*
 * MutexException.h
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#ifndef MUTEXEXCEPTION_H_
#define MUTEXEXCEPTION_H_

#include <string>
using std::string;

#include "../exception/NTmapException.h"

class MutexException : public NTmapException{
public:
	MutexException(const string & msg="Mutex Error Occurred");
	virtual ~MutexException() throw();
};

#endif /* MUTEXEXCEPTION_H_ */
