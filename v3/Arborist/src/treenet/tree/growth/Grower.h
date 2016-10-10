/*
 * Grower.h
 *
 *  Created on: Sept 9, 2016
 *      Author: grailet
 *
 * This partially abstract class defines the interface for children classes "[...]Grower" along 
 * a few common fields shared between these classes.
 */

#ifndef GROWER_H_
#define GROWER_H_

#include "../../TreeNETEnvironment.h"
#include "../../structure/SubnetSiteSet.h"
#include "../Soil.h"

class Grower
{
public:

    // Constructor, destructor
    Grower(TreeNETEnvironment *env);
    virtual ~Grower();
    
    // To be implemented by children classes
    virtual void prepare() = 0;
    virtual void grow() = 0;
    
    inline Soil* getResult() { return this->result; }

protected:

    // Fields set at instantiation
    TreeNETEnvironment *env;
    
    // Field to store the final result
    Soil *result;
    
};

#endif /* GROWER_H_ */
