/*
 * L3Device.h
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * This child of Vertice models an inferred router. The class notably adds a pointer to the 
 * corresponding Router object. The name "L3Device" was chosen to avoid any ambiguity with the 
 * existing Router class. Additionnaly, L3 devices might not always be typical routers nowadays, 
 * but hybrid/multitask devices.
 */

#ifndef L3DEVICE_H_
#define L3DEVICE_H_

#include "Vertice.h"
#include "../structure/Router.h"

class L3Device : public Vertice
{
public:
    
    /*
     * Just like subnets, routers/L3 devices can be "imaginary", i.e., not inferred but necessary 
     * to eventually obtain a connected component that is faithful to the input network tree.
     */
    
    enum DeviceType
    {
        T_INFERRED,
        T_IMAGINARY
    };

    const static string STRING_PREFIX;

    L3Device(unsigned int ID, Router *equivalent); // Inferred L3 device constructor
    L3Device(unsigned int ID); // Imaginary L3 device constructor
    ~L3Device();
    
    L3Device(L3Device *toCopy);
    
    inline unsigned short getType() { return type; }
    inline Router* getEquivalent() { return equivalent; }
    inline bool isImaginary() { return (type == T_IMAGINARY); }
    
    string toString();
    string toStringDetailed();

protected:

    unsigned short type;
    Router *equivalent;
};

#endif /* L3DEVICE_H_ */
