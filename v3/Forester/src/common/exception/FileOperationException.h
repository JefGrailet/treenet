/*
 * FileOperationException.h
 *
 *  Created on: Jul 28, 2008
 *      Author: root
 */

#ifndef FILEOPERATIONEXCEPTION_H_
#define FILEOPERATIONEXCEPTION_H_

#include "NTmapException.h"

class FileOperationException: public NTmapException {
public:
	FileOperationException(const string & msg="File Operation Error occurred");
	virtual ~FileOperationException()throw();
};

#endif /* FILEOPERATIONEXCEPTION_H_ */
