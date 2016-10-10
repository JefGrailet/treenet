/*
 * Uniform.h
 *
 *  Created on: Mar 13, 2010
 *      Author: root
 */

#ifndef UNIFORM_H_
#define UNIFORM_H_

#include "Distribution.h"
#include "../exception/InvalidParameterException.h"

class Uniform : public Distribution {
public:
	Uniform(PRNGenerator & prngen);
	Uniform();
	virtual ~Uniform();
	//returns random numbers between [lowerBound, upperBound] for non-negative lower and upper values valid when lower>=0
	double generateRandomValue(double lowerBound, double upperBound) throw(InvalidParameterException);
	unsigned long generateRandomValue(unsigned long lowerBound, unsigned long upperBound) throw(InvalidParameterException){return (unsigned long)generateRandomValue((double)lowerBound, (double)upperBound);};
};

#endif /* UNIFORM_H_ */
