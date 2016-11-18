/*
 * IPIDCollector.h
 *
 *  Created on: Feb 29, 2016
 *      Author: jefgrailet
 *
 * Provided a single IP, this class spawns as many threads (with a small delay between the launch 
 * of each) as the user wants IP-IDs for a same IP, each of them probing the same IP and 
 * retrieving an IP-ID along a time value. Once each thread got its IP-ID/time value pair, 
 * IPIDCollector sorts the results with respect to the time values and computes delays between 
 * each IP-ID, which are stored with the IP-ID values in the IP dictionnary (accessible through 
 * the environment object).
 *
 * The goal of this architecture is to ensure minimal delay between each probe targetting the 
 * destination IP. With the previous scheduling method, the IPs were probed by a single thread 
 * sequentially and a delay of 0,01s was added between each probe, therefore increasing delays and 
 * risks of having multiple rollovers in the IP-ID sequence, which might result in less accurate 
 * velocity estimation. The trade-off of this method is that less IPs can be probed at the same 
 * time, which should be compensated by the more accurate velocity estimation.
 *
 * To properly work, in addition to the target IP and TreeNETEnvironment object, this class is 
 * given an offset at initialization such that the threads it spawn does not collide when 
 * allocating sockets for network communication. A pointer to the parent AliasHintCollector 
 * object is also provided to get a probe token, as each probe from any thread should have its 
 * own unique token (needed for alias resolution with Ally method).
 */

#ifndef IPIDCOLLECTOR_H_
#define IPIDCOLLECTOR_H_

#include <list>
using std::list;

#include "../../common/thread/Runnable.h"
#include "../../prober/DirectProber.h"
#include "../TreeNETEnvironment.h"
#include "AliasHintCollector.h"

class IPIDCollector : public Runnable
{
public:

    // Constructor, destructor
    IPIDCollector(TreeNETEnvironment *env, 
                  AliasHintCollector *parent, 
                  InetAddress target, 
                  unsigned short offset);
    ~IPIDCollector();
    
    // Accesser to environment pointer
    inline TreeNETEnvironment *getEnvironment() { return this->env; }
    
    // Starts the probing
    void run();
    
    // Method to get the debug log of this thread
    inline string getDebugLog() { return this->log; }
    
private:

    // Pointer to the environment object (=> probing parameters)
    TreeNETEnvironment *env;
    
    // Pointer to the parent AliasHintCollector object
    AliasHintCollector *parent;

    // Own private fields
    InetAddress target;
    unsigned short offset;
    
    // Debug log
    string log;

};

#endif /* IPIDCOLLECTOR_H_ */
