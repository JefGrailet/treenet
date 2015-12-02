/*
 * MutexException.cpp
 *
 *  Created on: Jul 9, 2008
 *      Author: root
 */

#include "MutexException.h"

MutexException::MutexException(const string & msg):
	NTmapException(msg) {

}

MutexException::~MutexException()  throw(){
	// TODO Auto-generated destructor stub
}
