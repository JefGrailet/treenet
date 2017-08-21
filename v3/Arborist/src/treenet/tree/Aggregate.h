/*
 * Aggregate.h
 *
 *  Created on: Aug 21, 2017
 *      Author: jefgrailet
 *
 * This class models an aggregate of IP interfaces likely to be alias of each other, inferred 
 * through the construction of the network tree, as a kind of coarse-grained alias resolution (the 
 * application of the actual alias resolution techniques afterwards act as a fine-grained alias 
 * resolution). It gathers said interfaces, extracted from subnets, their common last hop(s), 
 * extracted from node labels, and the associated fingerprints/inferred routers obtained after 
 * the alias resolution.
 *
 * The progressive complexification of the interface of NetworkTreeNode to conduct a more refined 
 * alias resolution on multi-label nodes is the main motivation behind this additionnal class; 
 * otherwise everything would be in NetworkTreeNode just like before.
 */

#ifndef AGGREGATE_H_
#define AGGREGATE_H_

#include <list>
using std::list;

#include "../aliasresolution/Fingerprint.h"
#include "../structure/Router.h"

class Aggregate
{
public:

    Aggregate(InetAddress lastHop, list<InetAddress> interfaces);
    ~Aggregate();
    
    inline list<InetAddress> *getLastHops() { return &lastHops; }
    inline list<InetAddress> *getCandidates() { return &candidates; }
    inline list<Fingerprint> *getFingerprints() { return &fingerprints; }
    inline list<Router*> *getInferredRouters() { return &inferredRouters; }
    
    inline bool isSimple() { return lastHops.size() == 1; }
    inline InetAddress getFirstLastHop() { return lastHops.front(); }
    
    list<InetAddress> listAllInterfaces();
    
    static bool compare(Aggregate *a1, Aggregate *a2);
    
    // TODO: merging method
    
private:
    
    list<InetAddress> lastHops;
    list<InetAddress> candidates;
    
    /*
     * N.B.: there can be several last hops. It is indeed possible some IPs identified as last 
     * hops are aliasable too; in that case both aggregates should be merged together.
     */
    
    list<Fingerprint> fingerprints;
    list<Router*> inferredRouters;
};

#endif /* AGGREGATE_H_ */
