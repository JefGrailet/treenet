#include "SocketException.h"

SocketException::SocketException(const string & msg)
:NTmapException(msg)
{
}

SocketException::~SocketException() throw()
{
}
