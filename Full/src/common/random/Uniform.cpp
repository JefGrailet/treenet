/*
 * Uniform.cpp
 *
 *  Created on: Mar 13, 2010
 *      Author: root
 */

#include "Uniform.h"
#include <cmath>

Uniform::Uniform(PRNGenerator & prngen):
Distribution(prngen)
{

}

Uniform::Uniform():
Distribution()
{

}

Uniform::~Uniform() {}

double Uniform::generateRandomValue(double lowerBound, double upperBound) throw(InvalidParameterException){
	if(upperBound<=lowerBound){
		throw InvalidParameterException("upper bound must be greater than lower bound");
	}
	double difference=upperBound-lowerBound+1.0;
	return (prngen.getNextRandomNumber()*difference)+lowerBound;

}

