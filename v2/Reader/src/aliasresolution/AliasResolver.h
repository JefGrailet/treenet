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
 */

#ifndef ALIASRESOLVER_H_
#define ALIASRESOLVER_H_

#include "../utils/TreeNETEnvironment.h"
#include "../structure/Router.h"
#include "../structure/NetworkTreeNode.h"

class AliasResolver
{
public:

    /*
     * Max difference between 2 IP identifiers (for router inference) to associate their 
     * respective interfaces with Ally, maximum error while rounding y, z while computing the 
     * velocity of an IP ID counter and base tolerance value for association by velocity
     * (*2 for each 50).
     */
    
    static const unsigned short MAX_IP_ID_DIFFERENCE = 3268; // 0.05 * 65355 (max value)
    static const double MAX_VELOCITY_ERROR = 0.3;
    static const double BASE_VELOCITY_TOLERANCE = 5.0;
    
    // Possible results for Ally method
    enum AllyResults
    {
        ALLY_NO_SEQUENCE, // A token sequence could not be found
        ALLY_REJECTED, // The sequence exists, but Ally rejects it
        ALLY_ACCEPTED // The sequences exists and Ally acknowledges the association
    };

    // Constructor/destructor
    AliasResolver(TreeNETEnvironment *env);
    ~AliasResolver();
    
    // Setter for currentTTL
    inline void setCurrentTTL(unsigned char newTTL) { this->currentTTL = newTTL; }
    
    // Method to infer routers in an internal NetworkTreeNode (hints are obtained via IP Table)
    void resolve(NetworkTreeNode *internal);
    
private:

    // Pointer to the environment object (=> IP table)
    TreeNETEnvironment *env;
    
    // currentTTL field (identical to AliasHintCollector)
    unsigned char currentTTL;
    
    /*
     * Method to perform Ally method for alias resolution, i.e., if for 2 distinct IPs, we have
     * 3 IP IDs with increasing tokens which are also increasing themselves and close in values, 
     * 2 of them being from the first IP and the last one from the second, they are likely from 
     * the same router.
     *
     * @param IPTableEntry* ip1       The first IP to associate
     * @param IPTableEntry* ip2       The second IP to associate
     * @param unsigned short maxDiff  The largest gap to be accepted between 2 consecutive IDs
     * @return unsigned short         One of the results listed in "enum AllyResults"
     */
    
    unsigned short Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff);
    
    /*
     * Method to evaluate an IP ID counter, which amounts to computing its velocity and/or 
     * labelling it with a (unsigned short) class defined in IPTableEntry.h. It returns nothing.
     *
     * @param IPTableEntry* ip  The IP for which the IP ID counter needs to be evaluated
     */
    
    void evaluateIPIDCounter(IPTableEntry *ip);
    
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
    
    /*
     * Method to check if two distinct IPs (given as IPTableEntry objects) can be associated 
     * with the reverse DNS method.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if association is possible, false otherwise
     */
    
    bool reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2);
};

#endif /* ALIASRESOLVER_H_ */
