#include "SocketSendException.h"

SocketSendException::SocketSendException(const string &msg):SocketException(msg)
{
}

SocketSendException::~SocketSendException()throw()
{
}
