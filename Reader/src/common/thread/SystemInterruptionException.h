/*
 * SystemInterruptionException.h
 *
 *  Created on: Jul 12, 2008
 *      Author: root
 */

#ifndef SYSTEMINTERRUPTIONEXCEPTION_H_
#define SYSTEMINTERRUPTIONEXCEPTION_H_

#include <string>
using std::string;

#include "../exception/NTmapException.h"

class SystemInterruptionException: public NTmapException {
public:
	SystemInterruptionException(const string & msg="The operation is interrupted because of a system signal");
	virtual ~SystemInterruptionException() throw();
};

#endif /* SYSTEMINTERRUPTIONEXCEPTION_H_ */
