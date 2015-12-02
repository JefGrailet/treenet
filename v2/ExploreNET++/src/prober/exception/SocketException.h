#ifndef SOCKETEXCEPTION_H_
#define SOCKETEXCEPTION_H_

#include <string>
using std::string;

#include "../../common/exception/NTmapException.h"

class SocketException : public NTmapException
{
public:
	SocketException(const string &msg = "Socket Error Occurred");
	virtual ~SocketException() throw();
};

#endif /*SOCKETEXCEPTION_H_*/
