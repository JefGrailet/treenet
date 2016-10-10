/*
 * Climber.cpp
 *
 *  Created on: Sept 13, 2016
 *      Author: grailet
 *
 * Implements the class defined in Climber.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Climber.h"

Climber::Climber(TreeNETEnvironment *env)
{
    this->env = env;
}

Climber::~Climber()
{
    // Nothing is deleted, because any pointer points to data structures used elsewhere
}
