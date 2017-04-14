/*
 * AliasHintCollector.h
 *
 *  Created on: Feb 27, 2015
 *      Author: jefgrailet
 *
 * Provided a list of IPs to probe with ICMP, this class spawns a series a thread, each of them 
 * probing a single IP and retrieving the IP identifier found in the response. This IP is later 
 * written inside the InetAddress object which corresponds to the probed IP. AliasHintCollector also
 * associates a "probe token" to each IP, which is a number taken from a single counter 
 * (synchronized with a Mutex object), which is later used to locate the IP identifiers in time.
 *
 * This duo IP identifier/probe token is later used to resolve aliases in an internal node 
 * (obtained in the network tree), therefore allowing the inference of routers. The alias 
 * resolution itself is performed with the class AliasResolver.
 *
 * N.B.: here, the TTL of each probe is a constant. Indeed, TTL is not relevant for these new
 * probes. This constant is defined by the class AliasHintCollectorUnit.
 */

#ifndef ALIASHINTCOLLECTOR_H_
#define ALIASHINTCOLLECTOR_H_

#include <list>
using std::list;
#include <map>
using std::map;
using std::pair;

#include "../TreeNETEnvironment.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "IPIDTuple.h"

class AliasHintCollector
{
public:

    // Threshold for assuming that 2 consecutives IDs after sorting by IDs are from a same device
    static const unsigned short IPID_MAX_DIFF = 10;

    // Constructor, destructor
    AliasHintCollector(TreeNETEnvironment *env);
    ~AliasHintCollector();
    
    // Accesser to environment pointer
    inline TreeNETEnvironment *getEnvironment() { return this->env; }
    
    // Setters for the list of IPs to probe (depends on the internal tree node)
    inline void setIPsToProbe(std::list<InetAddress> IPsToProbe) { this->IPsToProbe = IPsToProbe; }
    
    // Method to start the probing (note: this empties the IPsToProbe list)
    void collect();
    
    // Method to get a token (used by IPIDUnit objects)
    unsigned long int getProbeToken();
    
    /*
     * Method to let external methods (collectHintsRecursive() in NetworkTree class) know if every 
     * step is announced in the console output.
     */
    
    inline bool isPrintingSteps() { return this->printSteps; }
    inline bool debugMode() { return this->debug; }
    
private:

    // Pointer to the environment object (=> probing parameters)
    TreeNETEnvironment *env;

    // Very own private fields
    std::list<InetAddress> IPsToProbe;
    unsigned long int tokenCounter;
    
    // Map to save collected IP-ID tuples before treating and storing them in the IP dictionnary
    map<InetAddress, IPIDTuple*> IPIDTuples;
    
    // Debug stuff
    bool printSteps, debug;

};

#endif /* ALIASHINTCOLLECTOR_H_ */
