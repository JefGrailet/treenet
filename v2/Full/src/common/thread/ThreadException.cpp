/*
 * ThreadException.cpp
 *
 *  Created on: Jul 8, 2008
 *      Author: root
 */

#include "ThreadException.h"

ThreadException::ThreadException(const string & msg):
	NTmapException(msg)
{

}

ThreadException::~ThreadException() throw() {
}
