/*
 * Climber.h
 *
 *  Created on: Sept 13, 2016
 *      Author: jefgrailet
 *
 * This partially abstract class defines the interface for a bunch of classes which act as "tree 
 * travellers" to perform various operations, such as visiting neighborhoods for alias resolution 
 * hint collection or simply printing out the tree.
 */

#ifndef CLIMBER_H_
#define CLIMBER_H_

#include "../../TreeNETEnvironment.h"
#include "../Soil.h"

class Climber
{
public:

    // Constructor, destructor
    Climber(TreeNETEnvironment *env);
    virtual ~Climber();
    
    // To be implemented by children classes
    virtual void climb(Soil *fromSoil) = 0;

protected:

    // Unique field: environment object
    TreeNETEnvironment *env;
    
};

#endif /* CLIMBER_H_ */
