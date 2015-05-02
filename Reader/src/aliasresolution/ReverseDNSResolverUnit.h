/*
 * ReverseDNSResolverUnit.h
 *
 *  Created on: Mar 4, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, performs reverse DNS on a single IP to get its host name, if 
 * such name exists. It later stores the result in the corresponding InetAddress object.
 *
 * As its name suggests, ReverseDNSResolverUnit is used by AliasResolver for its probing. However, 
 * unlike IPIDResolverUnit, it does no require a pointer to the calling AliasResolver object as 
 * there is no token mechanism here (the host name is supposed to be constant). Also, no prober 
 * object is involved neither.
 *
 * While one could argue that reverse DNS could be done sequentially, this class helps to 
 * run this part of the program in parallel, to speed up the whole procedure.
 */

#ifndef REVERSEDNSRESOLVERUNIT_H_
#define REVERSEDNSRESOLVERUNIT_H_

#include <list>
using std::list;

#include "../common/thread/Runnable.h"
#include "../common/inet/InetAddress.h"

class ReverseDNSResolverUnit : public Runnable
{
public:

    // Constructor
    ReverseDNSResolverUnit(InetAddress *IPToProbe);
    
    // Destructor and run method
    ~ReverseDNSResolverUnit();
    void run();
    
private:
    
    // Private field
    InetAddress *IPToProbe;

};

#endif /* REVERSEDNSRESOLVERUNIT_H_ */
