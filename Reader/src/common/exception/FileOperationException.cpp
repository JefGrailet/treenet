/*
 * FileOperationException.cpp
 *
 *  Created on: Jul 28, 2008
 *      Author: root
 */

#include "FileOperationException.h"

FileOperationException::FileOperationException(const string &msg):
NTmapException(msg)
{

}

FileOperationException::~FileOperationException() throw() {
}
