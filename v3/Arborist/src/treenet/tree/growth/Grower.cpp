/*
 * Grower.cpp
 *
 *  Created on: Sept 9, 2016
 *      Author: grailet
 *
 * Implements the class defined in Grower.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Grower.h"

Grower::Grower(TreeNETEnvironment *env)
{
    this->env = env;
}

Grower::~Grower()
{
    // Nothing is deleted, because any pointer points to data structures used elsewhere
}
