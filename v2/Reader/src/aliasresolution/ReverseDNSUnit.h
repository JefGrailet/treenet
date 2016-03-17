/*
 * ReverseDNSUnit.h
 *
 *  Created on: Mar 4, 2015
 *      Author: grailet
 *
 * This class, inheriting Runnable, performs reverse DNS on a single IP to get its host name, if 
 * such name exists.
 *
 * As its name suggests, ReverseDNSUnit is used by AliasHintCollector for its probing. However, 
 * unlike IPIDUnit, it does no require a pointer to the calling object as there is no token 
 * mechanism here (the host name is constant). Also, no prober object is involved neither.
 *
 * While one could argue that reverse DNS could be done sequentially, this class helps to 
 * run this part of the program in parallel, to speed up the whole procedure.
 */

#ifndef REVERSEDNSUNIT_H_
#define REVERSEDNSUNIT_H_

#include "../utils/TreeNETEnvironment.h"
#include "../common/thread/Runnable.h"
#include "../common/inet/InetAddress.h"

class ReverseDNSUnit : public Runnable
{
public:

    // Constructor
    ReverseDNSUnit(TreeNETEnvironment *env, InetAddress &IPToProbe);
    
    // Destructor and run method
    ~ReverseDNSUnit();
    void run();
    
private:
    
    // Private fields
    TreeNETEnvironment *env;
    InetAddress &IPToProbe;

};

#endif /* REVERSEDNSUNIT_H_ */
