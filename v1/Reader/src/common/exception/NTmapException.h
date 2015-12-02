/*
 * NTmapException.h
 *
 *  Created on: Jul 26, 2012
 *      Author: engin
 */

#ifndef NTMAPEXCEPTION_H_
#define NTMAPEXCEPTION_H_

#include <stdexcept>
using std::runtime_error;
#include <string>
using std::string;

class NTmapException : public runtime_error
{
public:
	NTmapException(const string & msg="Exception Occurred");
	virtual ~NTmapException()throw();
};

#endif /* NTMAPEXCEPTION_H_ */
