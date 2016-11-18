/*
 * ProbesDispatcher.h
 *
 *  Created on: Oct 30, 2014
 *      Author: jefgrailet
 *
 * Provided a list of IPs to probe with ICMP, this class is used to dispatch blocks of IPs (of a
 * fixed size) between several threads with their own prober instance in order to speed up the
 * probing of a large amount of IPs (such as a range).
 */

#ifndef PROBESDISPATCHER_H_
#define PROBESDISPATCHER_H_

#include <list>
using std::list;

#include "../../TreeNETEnvironment.h"

class ProbesDispatcher
{
public:

    static const unsigned short MINIMUM_IPS_PER_THREAD = 2;
    
    // Possible results upon completing the probing work
    enum DispatchResult
    {
        FOUND_ALTERNATIVE,
        FOUND_RESPONSIVE_IPS,
        FOUND_PROOF_TO_DISCARD_ALTERNATIVE,
        FOUND_NOTHING
    };

    // Constructor (args are for the ICMP probers), destructor
    ProbesDispatcher(TreeNETEnvironment *env, 
                     std::list<InetAddress> IPsToProbe,
                     unsigned char requiredTTL,
                     unsigned char alternativeTTL);
    ~ProbesDispatcher();
    
    // Dispatch method
    unsigned short dispatch();
    
    // Accesser to responsive IPs list and "foundAlternative" flag (for probe units)
    inline std::list<InetAddress> *getResponsiveIPs() { return &responsiveIPs; }
    inline bool hasFoundAlternative() { return foundAlternative; }
    inline bool ignoringAlternative() { return ignoreAlternative; }
    
    // Method to set "foundAlternative" and "ignoreAlternative" to true (used by probe units)
    inline void raiseFoundAlternativeFlag() { foundAlternative = true; }
    inline void raiseIgnoreAlternativeFlag() { ignoreAlternative = true; }
    
private:

    // Pointer to the environment object
    TreeNETEnvironment *env;

    // Very own private fields
    std::list<InetAddress> IPsToProbe, responsiveIPs;
    unsigned char requiredTTL, alternativeTTL;
    
    // Flag telling alternative has been found
    bool foundAlternative;
    
    // Flag telling to not look for alternative anymore
    bool ignoreAlternative;
};

#endif /* PROBESDISPATCHER_H_ */
