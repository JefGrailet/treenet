/*
 * AliasResolver.h
 *
 *  Created on: Oct 20, 2015
 *      Author: grailet
 *
 * In order to thoroughly refactor the alias resolution module of TreeNET, previous AliasResolver 
 * has been renamed AliasHintCollector (a more fitting name), while this new class indeed performs 
 * alias resolution by using the "hints" collected by the former class. The goal of having a class 
 * for this is to separate the different alias resolution techniques from other parts of the code 
 * where they do not really belong. For example, the actual router inference in TreeNET v1.0 took 
 * place in NetworkTreeNode class, which is not what this class was built for at first.
 *
 * In this version adapted for the Alias Resolution assessment version, the code is slightly 
 * simplified due to the removal of reverse DNS (here, we only care about the IP ID part). The env 
 * object is also used to get the maximum amount of IP ID counter rollovers (5 in TreeNET in any 
 * case) and its base tolerance while associating (5.0 in TreeNET, in any case), which are two 
 * parameters of this version.
 */

#ifndef ALIASRESOLVER_H_
#define ALIASRESOLVER_H_

#include "../TreeNETEnvironment.h"

class AliasResolver
{
public:

    /*
     * Max difference between 2 IP identifiers (for router inference) to associate their 
     * respective interfaces with Ally and maximum error while rounding y, z while computing the 
     * velocity of an IP ID counter.
     */
    
    static const unsigned short MAX_IP_ID_DIFFERENCE = 3268; // 0.05 * 65355 (max value)
    static const double MAX_VELOCITY_ERROR = 0.3;

    // Constructor/destructor
    AliasResolver(TreeNETEnvironment *env);
    ~AliasResolver();
    
    /*
     * Method to infer one or several routers from a list of IPs (= expected router). The results, 
     * as InferredRouter objects, are stored within the Router object (hence void return type).
     */
    
    void resolve(Router *router);
    
private:

    // Pointer to the environment object (=> IP table)
    TreeNETEnvironment *env;
    
    /*
     * Method to perform Ally method for alias resolution, i.e., if for 2 distinct IPs, we have
     * 3 IP IDs with increasing tokens which are also increasing themselves and close in values, 
     * 2 of them being from the first IP and the last one from the second, they are likely from 
     * the same router.
     *
     * @param IPTableEntry* ip1       The first IP to associate
     * @param IPTableEntry* ip2       The second IP to associate
     * @param unsigned short maxDiff  The largest gap to be accepted between 2 consecutive IDs
     * @return bool                   True if ip1 and ip2 should be associated
     */
    
    bool Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff);
    
    /*
     * Method to evaluate the velocity of an IP ID counter. It returns nothing, but gives a lower 
     * and upper bound on the velocity which are stored in the IPTableEntry object for convenience.
     *
     * @param IPTableEntry* ip  The IP for which the velocity is being evaluated
     */
    
    void computeVelocity(IPTableEntry *ip);
    
    /*
     * Method to check if the velocity ranges of two distinct IPs (given as IPTableEntry objects) 
     * overlap, if which case both IPs should be associated.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if both ranges overlap, false if not or if one or both IPs 
     *                           do not have a velocity range
     */
    
    bool velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2);

};

#endif /* ALIASRESOLVER_H_ */
