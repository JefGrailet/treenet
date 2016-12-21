/*
 * L2Device.h
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * This child of Vertice models an Ethernet switch (or any other kind of L2 device) in an ERS 
 * double bipartite graph (ERS standing for "Ethernet Switch - Router - Subnet"), which are purely 
 * imaginary in the current TreeNET but will eventually be inferred too.
 */

#ifndef L2DEVICE_H_
#define L2DEVICE_H_

#include "Vertice.h"

class L2Device : public Vertice
{
public:
    
    /*
     * Eventually, there will be inferred L2 devices. The enum below and the "type" field are just 
     * a manner of signaling this will eventually be the case.
     */
    
    enum DeviceType
    {
        T_INFERRED,
        T_IMAGINARY
    };

    const static string STRING_PREFIX;

    L2Device(unsigned int ID); // Imaginary L3 device constructor
    ~L2Device();
    
    L2Device(L2Device *toCopy);
    
    inline unsigned short getType() { return type; } // For copy-constructor (see L2Device.cpp)
    // inline bool isImaginary() { return (type == T_IMAGINARY); }
    
    string toString();
    string toStringDetailed();

protected:

    unsigned short type;
};

#endif /* L2DEVICE_H_ */
