/*
 * EmptyCollectionException.h
 *
 *  Created on: Jul 27, 2008
 *      Author: root
 */

#ifndef EMPTYCOLLECTIONEXCEPTION_H_
#define EMPTYCOLLECTIONEXCEPTION_H_

#include "NTmapException.h"

class EmptyCollectionException: public NTmapException {
public:
	EmptyCollectionException(const string & msg="The collection queried is empty");
	virtual ~EmptyCollectionException()throw();
};

#endif /* EMPTYCOLLECTIONEXCEPTION_H_ */
