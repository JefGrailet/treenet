/*
 * Neighborhood.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * This child of Vertice models a neighborhood. The class notably adds a pointer to the 
 * corresponding NetworkTreeNode object.
 *
 * N.B.: unlike Router and Subnet, a Neighborhood can never be "imaginary" (it is an abstract 
 * construction to begin with, and neighborhoods are always derived from some measurement and 
 * never "invented").
 */

#ifndef NEIGHBORHOOD_H_
#define NEIGHBORHOOD_H_

#include "Vertice.h"
#include "../tree/NetworkTreeNode.h"

class Neighborhood : public Vertice
{
public:

    static const string STRING_PREFIX;

    Neighborhood(unsigned int ID, NetworkTreeNode *equivalent);
    ~Neighborhood();
    
    Neighborhood(Neighborhood *toCopy);
    
    inline NetworkTreeNode* getEquivalent() { return equivalent; }
    
    string toString();
    string toStringDetailed();

protected:

    NetworkTreeNode *equivalent;
};

#endif /* NEIGHBORHOOD_H_ */
