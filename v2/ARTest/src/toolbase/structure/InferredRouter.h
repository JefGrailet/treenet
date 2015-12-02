/*
 * InferredRouter.h
 *
 *  Created on: Mar 1, 2015
 *      Author: grailet
 *
 * A simple class to represent an inferred router (during alias resolution). It only consists in 
 * a list of interfaces (as RouterInterface objects). It is stricly equivalent to the "Router" 
 * class in TreeNET. The addition of "Inferred" is there to distinguish it from the "Router" 
 * class which here denotes an expected router (i.e. from the data set on which TreeNET is being 
 * assessed).
 */

#ifndef INFERREDROUTER_H_
#define INFERREDROUTER_H_

#include <list>
using std::list;

#include "../../common/inet/InetAddress.h"
#include "RouterInterface.h"

class InferredRouter
{
public:

    InferredRouter();
    ~InferredRouter();
    
    // Accessor to the list
    inline list<RouterInterface> *getInterfacesList() { return &interfaces; }

    // Method to add a new interface to this router.
    void addInterface(InetAddress interface, unsigned short aliasMethod);
    
    // Method to get the amount of interfaces of this router.
    unsigned short getNbInterfaces();
    
private:

    // Interfaces are stored with a list
    list<RouterInterface> interfaces;
};

#endif /* INFERREDROUTER_H_ */
