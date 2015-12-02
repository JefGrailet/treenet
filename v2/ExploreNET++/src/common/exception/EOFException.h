/*
 * EOFException.h
 *
 *  Created on: Jul 16, 2012
 *      Author: engin
 */

#ifndef EOFEXCEPTION_H_
#define EOFEXCEPTION_H_

#include "NTmapException.h"

class EOFException : public NTmapException {
public:
	EOFException(const string & msg="End of file is reached");
	virtual ~EOFException()throw();
};

#endif /* EOFEXCEPTION_H_ */
