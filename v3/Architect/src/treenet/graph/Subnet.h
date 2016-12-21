/*
 * Subnet.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * This child of Vertice models an inferred subnet. The class notably adds a pointer to the 
 * corresponding SubnetSite object.
 */

#ifndef SUBNET_H_
#define SUBNET_H_

#include "Vertice.h"
#include "../structure/SubnetSite.h"

class Subnet : public Vertice
{
public:
    
    /*
     * A subnet can be "imaginary", i.e., it was not inferred but it is necessary to obtain a 
     * single connected component in the end that is faithful to the original network tree.
     */
    
    enum SubnetType
    {
        T_INFERRED,
        T_IMAGINARY
    };

    const static string STRING_PREFIX;

    Subnet(unsigned int ID, SubnetSite *equivalent); // Inferred subnet constructor
    Subnet(unsigned int ID); // Imaginary subnet constructor
    ~Subnet();
    
    Subnet(Subnet *toCopy);
    
    inline unsigned short getType() { return type; }
    inline SubnetSite* getEquivalent() { return equivalent; }
    inline bool isImaginary() { return (type == T_IMAGINARY); }
    
    string toString();
    string toStringDetailed();

protected:

    unsigned short type;
    SubnetSite *equivalent;
};

#endif /* SUBNET_H_ */
