/*
 * Distribution.cpp
 *
 *  Created on: Mar 13, 2010
 *      Author: root
 */

#include "Distribution.h"

Distribution::Distribution(PRNGenerator & prng):
prngen(prng)
{

}
Distribution::Distribution():
prngen(PRNGenerator())
{

}

Distribution::~Distribution() {
}
