/*
 * PRNGenerator.h
 *
 *  Created on: Jan 29, 2010
 *      Author: root
 *
 *  Pseudo Random Number Generator.
 *  Generates a pseudo random number sequence given the parameters s (seed), k, and m.
 */

#ifndef PRNGENERATOR_H_
#define PRNGENERATOR_H_

class PRNGenerator {
public:
	/**
	 * parameter s is the seed value, please change s for different sequences
	 */
	PRNGenerator(double s, double k=16807, double m=2147483647);
	PRNGenerator(double k=16807, double m=2147483647);
	virtual ~PRNGenerator();
	void resetSeed(double s);
	/**
	 * Returns the next pseudo random number in interval (0,1).
	 */
	double getNextRandomNumber();
protected:
	inline void updateNextSequenceNumber();
	double s;
	double k;
	double m;
};

#endif /* PRNGENERATOR_H_ */
