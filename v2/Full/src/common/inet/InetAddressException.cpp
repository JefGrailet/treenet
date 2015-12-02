/*
 * InetAddressException.cpp
 *
 *  Created on: Jul 5, 2008
 *      Author: root
 */

#include "InetAddressException.h"

InetAddressException::InetAddressException(const string &msg)
:NTmapException(msg){


}

InetAddressException::~InetAddressException() throw(){
	// TODO Auto-generated destructor stub
}
