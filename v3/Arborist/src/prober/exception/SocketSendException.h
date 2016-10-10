#ifndef SOCKETSENDEXCEPTION_H_
#define SOCKETSENDEXCEPTION_H_
#include <string>
using std::string;

#include "SocketException.h"

class SocketSendException : SocketException
{
public:
	SocketSendException(const string &msg = "Can NOT send packet");
	virtual ~SocketSendException() throw();
};

#endif /*SOCKETSENDEXCEPTION_H_*/
