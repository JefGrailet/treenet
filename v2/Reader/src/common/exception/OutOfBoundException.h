/*
 * OutOfBoundException.h
 *
 *  Created on: Jul 25, 2008
 *      Author: root
 */

#ifndef OUTOFBOUNDEXCEPTION_H_
#define OUTOFBOUNDEXCEPTION_H_
#include <string>
using std::string;

#include "NTmapException.h"

class OutOfBoundException : NTmapException{
public:
	OutOfBoundException(const string & msg="The element subscript(position) is NOT in the boundary of the collection");
	virtual ~OutOfBoundException()throw();
};

#endif /* OUTOFBOUNDEXCEPTION_H_ */
