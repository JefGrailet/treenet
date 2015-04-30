/*
 * EmptyCollectionException.cpp
 *
 *  Created on: Jul 27, 2008
 *      Author: root
 */

#include "EmptyCollectionException.h"

EmptyCollectionException::EmptyCollectionException(const string &msg):
NTmapException(msg)
{

}

EmptyCollectionException::~EmptyCollectionException() throw(){
	// TODO Auto-generated destructor stub
}
