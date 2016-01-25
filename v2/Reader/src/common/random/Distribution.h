/*
 * Distribution.h
 *
 *  Created on: Mar 13, 2010
 *      Author: root
 */

#ifndef DISTRIBUTION_H_
#define DISTRIBUTION_H_

#include "../random/PRNGenerator.h"

class Distribution {
public:
	Distribution(PRNGenerator &prngen);
	Distribution();
	virtual ~Distribution();
protected:
	PRNGenerator prngen;
};

#endif /* DISTRIBUTION_H_ */
