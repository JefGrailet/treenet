/*
 * SubnetRefiner.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: grailet
 *
 * Implements the class defined in SubnetRefiner.h (see this file to learn further about the 
 * goals of such class).
 */

#include <list>
using std::list;
#include <math.h>

#include "SubnetRefiner.h"
#include "../structure/SubnetSiteNode.h"
#include "../../prober/structure/ProbeRecord.h"

SubnetRefiner::SubnetRefiner(ostream *o,
                             SubnetSiteSet *s, 
                             InetAddress &lIPa,
                             string &msg,
                             bool uffID,
                             const TimeVal &tp,
                             const TimeVal &prpp,
                             unsigned short lbii,
                             unsigned short ubii,
                             unsigned short lbis,
                             unsigned short ubis,
                             unsigned short mt):
out(o),         
set(s),
localIPAddress(lIPa),
attentionMessage(msg),
useFixedFlowID(uffID),
timeoutPeriod(tp),
probeRegulatorPausePeriod(prpp),
lowerBoundICMPid(lbii),
upperBoundICMPid(ubii),
lowerBoundICMPseq(lbis),
upperBoundICMPseq(ubis),
maxThreads(mt)
{
}

SubnetRefiner::~SubnetRefiner()
{
    set = NULL;
}

void SubnetRefiner::expand(SubnetSite *ss)
{
    (*out) << "Expanding " << ss->getInferredNetworkAddressString() << endl;

    unsigned char initialPrefix = ss->getInferredSubnetPrefixLength();
    
    // We computes the initial borders as well as the mask of known IPs
    InetAddress initLowerBorder, initUpperBorder;
    unsigned int initSize = (unsigned int) pow(2.0, (double) (32 - (unsigned int) initialPrefix));
    if(initialPrefix == 32)
    {
        initLowerBorder = ss->getPivotAddress();
        initUpperBorder = ss->getPivotAddress();
    }
    else
    {
        NetworkAddress na = ss->getInferredNetworkAddress();
        initLowerBorder = na.getLowerBorderAddress();
        initUpperBorder = na.getUpperBorderAddress();
    }
    
    unsigned short knownIPs[initSize];
    for(unsigned int i = 0; i < initSize; i++)
        knownIPs[i] = 0;
    
    std::list<SubnetSiteNode*> *IPsList = ss->getSubnetIPList();
    unsigned short nbValidIPs = 0;
    for(list<SubnetSiteNode*>::iterator it = IPsList->begin(); it != IPsList->end(); ++it)
    {
        /*
         * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
         * This condition is present in the original ExploreNET.
         */
    
        if((*it)->prefix >= ss->getInferredSubnetPrefixLength())
        {
            knownIPs[(*it)->ip.getULongAddress() - initLowerBorder.getULongAddress()] = 1;
            nbValidIPs++;
        }
    }
        
    // Now we get what TTL we should use.
    bool beforeAndAfter = (nbValidIPs == 1);
    unsigned char reqTTL = ss->getRefinementGreatestTTL();
    
    // Reduces prefix until a limit or until contra-pivot is found
    unsigned char prefix = initialPrefix;
    InetAddress previousLowerBorder(0), previousUpperBorder(0);
    while(prefix >= SubnetRefiner::LOWEST_PREFIX_ALLOWED)
    {
        // Borders of the current subnet
        InetAddress lowerBorder, upperBorder;
        if(prefix < 32)
        {
            NetworkAddress na(ss->getPivotAddress(), prefix);
            lowerBorder = na.getLowerBorderAddress();
            upperBorder = na.getUpperBorderAddress();
        }
        else
        {
            lowerBorder = initLowerBorder;
            upperBorder = initUpperBorder;
        }
        
        if(!set->isCompatible(lowerBorder, upperBorder, reqTTL, beforeAndAfter))
        {
            (*out) << lowerBorder << "/" << (unsigned short) prefix 
            << " collides with other inferred subnets while having incompatible TTLs" << endl;
            
            (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix 
            << " marked as shadow subnet" << endl << endl;
            
            ss->setRefinementStatus(SubnetSite::SHADOW_SUBNET);
            return;
        }
        
        (*out) << "Checking " << lowerBorder << "/" << (unsigned short) prefix << endl;
        
        std::list<InetAddress> IPsToProbe;
        for(InetAddress i = lowerBorder; i <= upperBorder; i++)
        {
            // Avoids probing known IPs/already probed IPs
            if(prefix == initialPrefix)
            {
                if(knownIPs[i.getULongAddress() - initLowerBorder.getULongAddress()] == 1)
                {
                    continue;
                }
            }
            else if(i >= previousLowerBorder && i <= previousUpperBorder)
            {
                continue;
            }
            
            IPsToProbe.push_back(i);
        }
        
        /*
         * ProbesDispatcher will dispatch the list of IPs to probe between up to maxThreads
         * thread(s) in order to speed up the refinement step.
         *
         * N.B.: alternativeTTL is used for a second probe on a same IP; when this probe succeeds 
         * in obtaining echo reply, dispatcher will set a flag to force all threads to terminate. 
         * It is not used anymore after a probe with regular TTL (here, reqTTL - 1) succeeds.
         * 
         * In this context, it is used when the subnet IP list contains only one IP to check 
         * that this IP is not a contra-pivot itself.
         */
         
        unsigned char alternativeTTL = 0;
        if(beforeAndAfter)
            alternativeTTL = reqTTL + 1;
        
        ProbesDispatcher *dispatcher = new ProbesDispatcher(IPsToProbe,
                                                            reqTTL - 1,
                                                            alternativeTTL,
                                                            maxThreads,
                                                            localIPAddress,
                                                            attentionMessage,
                                                            useFixedFlowID,
                                                            timeoutPeriod,
                                                            probeRegulatorPausePeriod,
                                                            lowerBoundICMPid,
                                                            upperBoundICMPid,
                                                            lowerBoundICMPseq,
                                                            upperBoundICMPseq);
        
        unsigned short result = dispatcher->dispatch();
        
        /*
         * Several cases:
         * 
         * 1) The list contained only one IP (signaled to dispatcher by alternativeTTL being not
         *    null) and we found out that it was the contra-pivot (result is FOUND_ALTERNATIVE). 
         *    Refiner therefore adds the pivot found during expansion to the list, save the new 
         *    prefix and labels the subnet as ACCURATE.
         * 2) Refiner found one or more responsive IPs at reqTTL - 1: in both case, the new prefix 
         *    is saved and the first responsive IP of the list is taken as contra-pivot. However:
         *    -when there is only one IP, subnet is labeled as ACCURATE;
         *    -otherwise, it is considered as ODD.
         */
        
        if(result == ProbesDispatcher::FOUND_ALTERNATIVE)
        {
            InetAddress pivot = dispatcher->getResponsiveIPs()->front();
            
            InetAddress contrapivot = IPsList->front()->ip;
            ss->setRefinementStatus(SubnetSite::ACCURATE_SUBNET);
            ss->setRefinementGreatestTTL(reqTTL + 1);
            ss->setRefinementContrapivot(contrapivot);
            ss->setInferredSubnetPrefixLength(prefix);
            ss->insert(new SubnetSiteNode(pivot, 
                                          prefix, 
                                          reqTTL + 1, 
                                          SubnetSiteNode::UNKNOWN_ALIAS_SX));
            
            (*out) << "Known IP " << contrapivot << " was the contra-pivot" << endl << endl;
            delete dispatcher;
            return;
        }
        else if(result == ProbesDispatcher::FOUND_RESPONSIVE_IPS)
        {
            std::list<InetAddress> *candidates = dispatcher->getResponsiveIPs();
            candidates->sort(InetAddress::smaller);
            size_t nbCandidates = candidates->size();
            
            // Extreme case where there are more than 5 candidates
            if(nbCandidates >= MAX_CONTRAPIVOT_CANDIDATES)
            {
                (*out) << lowerBorder << "/" << (unsigned short) prefix 
                << " has too many candidates for contra-pivot" << endl;
                
                (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix 
                << " marked as shadow subnet" << endl << endl;
                
                ss->setRefinementStatus(SubnetSite::SHADOW_SUBNET);
                delete dispatcher;
                return;
            }
            
            // Selected contra-pivot is by default the lowest address (first in the list)
            InetAddress contrapivot = candidates->front();
            ss->setRefinementShortestTTL(reqTTL - 1);
            ss->setRefinementContrapivot(contrapivot);
            ss->setInferredSubnetPrefixLength(prefix);
            ss->insert(new SubnetSiteNode(contrapivot, 
                                          prefix, 
                                          reqTTL - 1, 
                                          SubnetSiteNode::UNKNOWN_ALIAS_SX));
            
            if(nbCandidates == 1)
            {
                ss->setRefinementStatus(SubnetSite::ACCURATE_SUBNET);
                (*out) << "Contra-pivot is " << contrapivot << endl << endl;
                delete dispatcher;
                return;
            }
            else
            {
                ss->setRefinementStatus(SubnetSite::ODD_SUBNET);
                (*out) << "This is an odd subnet:" << endl;
                bool guardian = false;
                std::list<InetAddress>::iterator listBegin = candidates->begin();
                std::list<InetAddress>::iterator listEnd = candidates->end();
                for(std::list<InetAddress>::iterator it = listBegin; it != listEnd; ++it)
                {
                    (*out) << (*it) << endl;
                    
                    /*
                     * IPs besides the first candidate are inserted as well with right TTL, 
                     * otherwise it will lead to errors during the filling step (previous 
                     * candidates for contra-pivot would be listed as pivots because they will 
                     * reply to reqTTL anyway since they are at reqTTL - 1).
                     */
                    
                    if(guardian)
                    {
                        ss->insert(new SubnetSiteNode((*it), 
                                                      prefix, 
                                                      reqTTL - 1, 
                                                      SubnetSiteNode::UNKNOWN_ALIAS_SX));
                    }
                    guardian = true;
                }
                (*out) << endl;
                delete dispatcher;
                return;
            }
        }
        else if(result == ProbesDispatcher::FOUND_PROOF_TO_DISCARD_ALTERNATIVE)
        {
            beforeAndAfter = false;
        }
        
        delete dispatcher;
        
        prefix--;
        previousLowerBorder = lowerBorder;
        previousUpperBorder = upperBorder;
    }
    
    (*out) << "Subnet is undefined" << endl << endl;
    
    ss->setRefinementStatus(SubnetSite::UNDEFINED_SUBNET);
}

void SubnetRefiner::fill(SubnetSite *ss)
{
    (*out) << "Filling " << ss->getInferredNetworkAddressString() << endl;
    
    InetAddress lowerBorder, upperBorder;
    std::list<SubnetSiteNode*> *IPsList = ss->getSubnetIPList();
    unsigned char prefixLength = ss->getInferredSubnetPrefixLength();
    if(prefixLength > 32)
        return;
    
    // For some reason, this occured on PlanetLab... by security, we stop here.
    if(prefixLength == 0)
        return;
    
    // Checks if we dont have already the maximum number of elements
    unsigned int maxSize = (unsigned int) pow(2.0, (double) (32 - (unsigned int) prefixLength));
    if(maxSize == IPsList->size())
    {
        return;
    }
    
    // Array to quickly check if an IP is known or not
    unsigned short knownIPs[maxSize];
    for(unsigned int i = 0; i < maxSize; i++)
        knownIPs[i] = 0;

    /*
     * By design, there should be no /32 subnet any longer by now, so using NetworkAddress 
     * directly like this should not cause any problem like it could in expand().
     */

    NetworkAddress na = ss->getInferredNetworkAddress();
    lowerBorder = na.getLowerBorderAddress();
    upperBorder = na.getUpperBorderAddress();
    
    // Goes through the list to find TTL for pivot node and determine which IP is already known
    unsigned char reqTTL = 0;
    for(list<SubnetSiteNode*>::iterator it = IPsList->begin(); it != IPsList->end(); ++it)
    {
        SubnetSiteNode *cur = (*it);
        if(cur == NULL) // By security
            continue;
        
        if(cur->ip == ss->getPivotAddress())
        {
            reqTTL = cur->TTL;
        }
        knownIPs[cur->ip.getULongAddress() - lowerBorder.getULongAddress()] = 1;
    }
    
    // Computes list of missing nodes
    std::list<InetAddress> IPsToReprobe;
    for(InetAddress i = lowerBorder; i <= upperBorder; i++)
    {
        if(knownIPs[i.getULongAddress() - lowerBorder.getULongAddress()] == 0)
        {
            IPsToReprobe.push_back(i);
        }
    }
    
    // Dispatches probes
    ProbesDispatcher *dispatcher = new ProbesDispatcher(IPsToReprobe,
                                                        reqTTL,
                                                        0,
                                                        maxThreads,
                                                        localIPAddress,
                                                        attentionMessage,
                                                        useFixedFlowID,
                                                        timeoutPeriod,
                                                        probeRegulatorPausePeriod,
                                                        lowerBoundICMPid,
                                                        upperBoundICMPid,
                                                        lowerBoundICMPseq,
                                                        upperBoundICMPseq);
        
    unsigned short result = dispatcher->dispatch();
    if(result == ProbesDispatcher::FOUND_RESPONSIVE_IPS)
    {
        std::list<InetAddress> *missingIPs = dispatcher->getResponsiveIPs();
        std::list<InetAddress>::iterator listBegin = missingIPs->begin();
        std::list<InetAddress>::iterator listEnd = missingIPs->end();
        for(std::list<InetAddress>::iterator it = listBegin; it != listEnd; ++it)
        {
            ss->insert(new SubnetSiteNode(InetAddress(*it), 
                                          prefixLength, 
                                          reqTTL, 
                                          SubnetSiteNode::UNKNOWN_ALIAS_SX));
        }
    }
    
    delete dispatcher;
    
    // Sorts the final list
    IPsList->sort(SubnetSiteNode::smaller);
}

void SubnetRefiner::shadowExpand(SubnetSite *ss)
{
    (*out) << "Expanding shadow subnet " << ss->getInferredNetworkAddressString() << endl;

    unsigned char initialPrefix = ss->getInferredSubnetPrefixLength();
    
    // We computes the initial borders
    InetAddress initLowerBorder, initUpperBorder;
    if(initialPrefix == 32)
    {
        initLowerBorder = ss->getPivotAddress();
        initUpperBorder = ss->getPivotAddress();
    }
    else
    {
        NetworkAddress na = ss->getInferredNetworkAddress();
        initLowerBorder = na.getLowerBorderAddress();
        initUpperBorder = na.getUpperBorderAddress();
    }
    
    // Determines "beforeAndAfter" boolean (same spirit as in expand())
    bool beforeAndAfter = true;
    unsigned short nbValid = 0;
    std::list<SubnetSiteNode*> *IPsList = ss->getSubnetIPList();
    for(list<SubnetSiteNode*>::iterator it = IPsList->begin(); it != IPsList->end(); ++it)
    {
        if((*it)->prefix >= ss->getInferredSubnetPrefixLength())
        {
            nbValid++;
            if(nbValid > 1)
                beforeAndAfter = false;
        }
    }
    
    // Reduces prefix until the expanded shadow subnet collides with accurate/odd subnets
    unsigned char prefix = initialPrefix;
    while(prefix >= SubnetRefiner::LOWEST_PREFIX_ALLOWED)
    {
        // Borders of the current subnet
        InetAddress lowerBorder, upperBorder;
        if(prefix < 32)
        {
            NetworkAddress na(ss->getPivotAddress(), prefix);
            lowerBorder = na.getLowerBorderAddress();
            upperBorder = na.getUpperBorderAddress();
        }
        else
        {
            lowerBorder = initLowerBorder;
            upperBorder = initUpperBorder;
        }
        
        // Re-using isCompatible() method for convenience
        if(!set->isCompatible(lowerBorder, 
                              upperBorder, 
                              ss->getRefinementShortestTTL(), 
                              beforeAndAfter))
        {
            (*out) << lowerBorder << "/" << (unsigned short) prefix 
            << " collides with other inferred subnets" << endl;
            
            (*out) << initLowerBorder << "/" << (unsigned short) (prefix + 1)
            << " is the best estimation of this shadow subnet" << endl << endl;
            
            ss->setInferredSubnetPrefixLength(prefix + 1);
            return;
        }
        
        (*out) << lowerBorder << "/" << (unsigned short) prefix << " is possible" << endl;
        
        prefix--;
    }
    
    unsigned char finalPrefix = SubnetRefiner::LOWEST_PREFIX_ALLOWED;
    (*out) << "Subnet is of maximum size (/" << (unsigned short) finalPrefix;
    (*out) << ")" << endl << endl;
    
    ss->setInferredSubnetPrefixLength(finalPrefix);
}
