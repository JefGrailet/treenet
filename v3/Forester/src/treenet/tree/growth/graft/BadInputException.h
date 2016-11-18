/*
 * BadInputException.h
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * Exception thrown when less than 2 datasets were successfully while setting up a Grafter object.
 */

#ifndef BADINPUTEXCEPTION_H_
#define BADINPUTEXCEPTION_H_

#include <stdexcept>
using std::runtime_error;
#include <string>
using std::string;

class BadInputException : public runtime_error
{
public:
	BadInputException(const string &msg="Cannot graft if less than 2 datasets were parsed");
	virtual ~BadInputException() throw();
};

#endif /* BADINPUTEXCEPTION_H_ */
