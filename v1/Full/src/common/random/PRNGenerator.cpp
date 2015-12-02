/*
 * PRNGenerator.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: root
 */

#include "PRNGenerator.h"

#include <cmath>
#include <cstdlib>
#include <ctime>


PRNGenerator::PRNGenerator(double sa, double ka, double ma):
s(sa),
k(ka),
m(ma)
{

}

PRNGenerator::PRNGenerator(double ka, double ma):
k(ka),
m(ma)
{
	srand ( time(0) );
	s = rand()%10000;

}

PRNGenerator::~PRNGenerator() {}


void PRNGenerator::resetSeed(double s){
	this->s=s;
}

void PRNGenerator::updateNextSequenceNumber(){
	s=fmod(k*s,m);
}
double PRNGenerator::getNextRandomNumber(){
	updateNextSequenceNumber();
	return s/m;
}
