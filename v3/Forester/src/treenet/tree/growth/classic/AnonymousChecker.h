/*
 * AnonymousChecker.h
 *
 *  Created on: Feb 15, 2017
 *      Author: jefgrailet
 *
 * AnonymousChecker is designed to schedule new probing towards certain subnets which the route 
 * obtained through Paris traceroute is incomplete, i.e., there are anonymous hops, or rather 
 * steps in the route where there was a timeout during probing instead of a regular reply. Such 
 * anonymous hops can be caused by several factors:
 * -either the corresponding router is configured to not reply to such probes (pure anonymous 
 *  router), 
 * -either the corresponding router is rate-limited and missing hops can be solved by re-probing 
 *  with careful timing, 
 * -either a firewall dropped the packets to prevent router discovery.
 *
 * Of course, AnonymousChecker aims at "solving" the second case, which can not always be solved 
 * through offline correction as already applied by TreeNET. To do so, it carefully analyzes the 
 * current routes to isolate those which require additionnal probing (if several incomplete routes 
 * are similar and could be fixed offline, then only one probe is needed) then spread the work 
 * among several probing threads (see the class AnonymousCheckUnit).
 */

#ifndef ANONYMOUSCHECKER_H_
#define ANONYMOUSCHECKER_H_

#include <map>
using std::map;
using std::pair;

#include "../../../TreeNETEnvironment.h"

class AnonymousChecker
{
public:

    const static unsigned int THREAD_PROBES_PER_HOUR = 1800; // Max. of probes per thread/hour
    const static unsigned short MIN_THREADS = 4; // To speed up things for low amount of targets
    const static unsigned short MAX_THREADS = 16; // To avoid being too aggressive, too

    // Constructor, destructor
    AnonymousChecker(TreeNETEnvironment *env);
    ~AnonymousChecker();
    
    inline unsigned int getTotalAnonymous() { return this->totalAnonymous; }
    inline unsigned int getTotalSolved() { return this->totalSolved; }
    
    unsigned int getTotalFullyRepaired(); // To call AFTER probe() (like getTotalSolved())
    float getRatioSolvedHops(); // Idem
    void reload();
    
    // Callback method (for children threads)
    void callback(SubnetSite *target, unsigned short hop, InetAddress solution);
    
    // Starts the probing
    void probe();
    
private:

    // Pointer to the environment (provides access to the subnet set and prober parameters)
    TreeNETEnvironment *env;
    
    // Count of hops to solve and solved hops
    unsigned int totalAnonymous;
    unsigned int totalSolved;
    
    /*
     * Two lists: one to list all subnets with incomplete routes at loading, and another to list 
     * the subnets which are used as targets for the probing. Not all incomplete routes are probed 
     * again because route similarities allow offline fix after solving an anonymous hop. Say 3 
     * subnets have the route steps ..., 1.2.3.4, 0.0.0.0, * 5.6.7.8, ... at the same positions, 
     * then only one should be re-probed with the proper TTL to try to solve the anonymous step.
     */
    
    list<SubnetSite*> incompleteSubnets;
    list<SubnetSite*> targetSubnets;
    
    /*
     * Following map is used to match a target subnets with the subnets that share route 
     * similarities allowing a offline fix.
     */
    
    map<SubnetSite*, list<SubnetSite*> > toFixOffline;
    
    static bool similarAnonymousHops(SubnetSite *ss1, SubnetSite *ss2); // True if fit for fix
    void loadTargets(); // Lists targets
    
}; 

#endif /* ANONYMOUSCHECKER_H_ */
