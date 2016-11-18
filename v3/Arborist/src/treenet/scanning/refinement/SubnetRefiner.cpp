/*
 * SubnetRefiner.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetRefiner.h (see this file to learn further about the 
 * goals of such class).
 */

#include <list>
using std::list;
#include <math.h>

#include "SubnetRefiner.h"
#include "../../structure/SubnetSiteNode.h"

SubnetRefiner::SubnetRefiner(TreeNETEnvironment *env)
{
    this->env = env;
    this->printSteps = false;
    this->debug = false;
    
    switch(env->getDisplayMode())
    {
        case TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE:
        case TreeNETEnvironment::DISPLAY_MODE_VERBOSE:
            this->printSteps = true;
            break;
        case TreeNETEnvironment::DISPLAY_MODE_DEBUG:
            this->printSteps = true;
            this->debug = true;
            break;
        default:
            break;
    }
}

SubnetRefiner::~SubnetRefiner()
{
}

void SubnetRefiner::expand(SubnetSite *ss)
{
    // For security
    if(ss == NULL)
        return;

    // Relevant stuff from env
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    NetworkAddress LAN = env->getLAN();
    InetAddress LANLowerBorder = LAN.getLowerBorderAddress();
    InetAddress LANUpperBorder = LAN.getUpperBorderAddress();

    // Starts printing out
    (*out) << "Expanding " << ss->getInferredNetworkAddressString() << "...";
    if(printSteps)
        (*out) << "\n";
    else
        (*out) << " ";
    (*out) << std::flush;

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
    unsigned char reqTTL = ss->getGreatestTTL();
    
    // Debug message stating the alternative TTL will be checked as well.
    if(debug && beforeAndAfter)
    {
        (*out) << "\n";
        (*out) << "Note: the single known IP (at TTL) might be the contra-pivot interface itself.\n";
        (*out) << "Candidate IPs will therefore also be probed with TTL + 1 to check this.\n";
        (*out) << "This will stop if one IP replying to TTL + 1 also replies to TTL as well.\n";
        (*out) << endl;
    }
    
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
        
        // SHADOW subnet if it encompasses LAN
        if(lowerBorder <= LANLowerBorder && upperBorder >= LANUpperBorder)
        {
            if(printSteps)
            {
                (*out) << lowerBorder << "/" << (unsigned short) prefix;
                (*out) << " collides with the LAN of the vantage point.\n";
                (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix;
                (*out) << " will be marked as a shadow subnet.\n" << endl;
            }
            else
            {
                (*out) << "It could not be successfully expanded (VP LAN collision) and will be ";
                (*out) << "marked as a shadow subnet." << endl;
            }
            
            ss->setStatus(SubnetSite::SHADOW_SUBNET);
            return;
        }
        
        if(!subnetSet->isCompatible(lowerBorder, upperBorder, reqTTL, beforeAndAfter, false))
        {
            if(printSteps)
            {
                (*out) << lowerBorder << "/" << (unsigned short) prefix << " collides with ";
                (*out) << "other inferred subnets while having incompatible TTLs.\n";
                (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix;
                (*out) << " will be marked as a shadow subnet.\n" << endl;
            }
            else
            {
                (*out) << "It could not be successfully expanded (bad collision) and will be ";
                (*out) << "marked as a shadow subnet." << endl;
            }
            
            ss->setStatus(SubnetSite::SHADOW_SUBNET);
            return;
        }
        
        if(printSteps)
        {
            (*out) << "Testing " << lowerBorder << "/" << (unsigned short) prefix << "..." << endl;
        }
        
        std::list<InetAddress> IPsToProbe;
        IPLookUpTable *table = env->getIPTable();
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
            // New improvement (Oct 8, 2015): skips IPs which are known to be unresponsive
            else if(table->lookUp(i) == NULL)
            {
                continue;
            }
            
            IPsToProbe.push_back(i);
        }
        
        /*
         * ProbesDispatcher will dispatch the list of IPs to probe between up to maxThreads
         * thread(s) in order to speed up the refinement step.
         *
         * N.B.: altTTL is used for a second probe on a same IP; when this probe succeeds in 
         * obtaining echo reply, dispatcher will set a flag to force all threads to terminate. 
         * It is not used anymore after a probe with regular TTL (here, reqTTL - 1) succeeds.
         * 
         * In this context, it is used when the subnet IP list contains only one IP to check 
         * that this IP is not a contra-pivot itself.
         */
         
        unsigned char altTTL = 0;
        if(beforeAndAfter)
            altTTL = reqTTL + 1;
     
        ProbesDispatcher *dispatcher = new ProbesDispatcher(env, IPsToProbe, reqTTL - 1, altTTL);
        
        unsigned short result = 0;
        try
        {
            result = dispatcher->dispatch();
        }
        catch(StopException &se)
        {
            delete dispatcher;
            
            if(printSteps)
            {
                (*out) << "Critical failure occurred while probing.\n";
                (*out) << "The subnet will not be further expanded or refined.\n" << endl;
            }
            else
            {
                (*out) << "Critical failure occurred while probing (stopped expansion)." << endl;
            }
            
            throw;
        }
        
        /*
         * Several cases:
         * 
         * 1) The list contained only one IP (signaled to dispatcher by altTTL being not null) 
         *    and we found out that it was the contra-pivot (result is FOUND_ALTERNATIVE). 
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
            ss->setStatus(SubnetSite::ACCURATE_SUBNET);
            ss->setGreatestTTL(reqTTL + 1);
            ss->setContrapivot(contrapivot);
            ss->setInferredSubnetPrefixLength(prefix);
            ss->insert(new SubnetSiteNode(pivot, 
                                          prefix, 
                                          reqTTL + 1, 
                                          SubnetSiteNode::UNKNOWN_ALIAS_SX));
            
            if(printSteps)
            {
                (*out) << "Known IP " << contrapivot << " was the contra-pivot IP.\n";
                (*out) << "Final result is " << lowerBorder << "/" << (unsigned short) prefix;
                (*out) << " (accurate subnet).\n" << endl;
            }
            else
            {
                (*out) << "Successfully expanded into " << lowerBorder << "/";
                (*out) << (unsigned short) prefix << " (accurate subnet)." << endl;
            }
            
            delete dispatcher;
            return;
        }
        else if(result == ProbesDispatcher::FOUND_RESPONSIVE_IPS)
        {
            std::list<InetAddress> *candidates = dispatcher->getResponsiveIPs();
            candidates->sort(InetAddress::smaller);
            size_t nbCandidates = candidates->size();
            
            // Extreme case where there are more than 5 candidates
            if(nbCandidates > MAX_CONTRAPIVOT_CANDIDATES)
            {
                if(printSteps)
                {
                    (*out) << lowerBorder << "/" << (unsigned short) prefix;
                    (*out) << " has too many contra-pivot candidates (more than ";
                    (*out) << MAX_CONTRAPIVOT_CANDIDATES << ").\n";
                    (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix;
                    (*out) << " will be marked as a shadow subnet.\n" << endl;
                }
                else
                {
                    (*out) << "It could not be successfully expanded (too many candidates) and ";
                    (*out) << "will be marked as a shadow subnet." << endl;
                }
                
                ss->setStatus(SubnetSite::SHADOW_SUBNET);
                delete dispatcher;
                return;
            }
            
            // Selected contra-pivot is by default the lowest address (first in the list)
            InetAddress contrapivot = candidates->front();
            ss->setShortestTTL(reqTTL - 1);
            ss->setContrapivot(contrapivot);
            ss->setInferredSubnetPrefixLength(prefix);
            ss->insert(new SubnetSiteNode(contrapivot, 
                                          prefix, 
                                          reqTTL - 1, 
                                          SubnetSiteNode::UNKNOWN_ALIAS_SX));
            
            if(nbCandidates == 1)
            {
                ss->setStatus(SubnetSite::ACCURATE_SUBNET);
                
                if(printSteps)
                {
                    (*out) << "Found a valid contra-pivot IP: " << contrapivot << ".\n";
                    (*out) << "Final result is " << lowerBorder << "/" << (unsigned short) prefix;
                    (*out) << " (accurate subnet).\n" << endl;
                }
                else
                {
                    (*out) << "Successfully expanded into " << lowerBorder << "/";
                    (*out) << (unsigned short) prefix << " (accurate subnet)." << endl;
                }
                
                delete dispatcher;
                return;
            }
            else
            {
                ss->setStatus(SubnetSite::ODD_SUBNET);
                
                if(printSteps)
                    (*out) << "Found several sound contra-pivot candidates:\n";
                
                bool guardian = false;
                std::list<InetAddress>::iterator listBegin = candidates->begin();
                std::list<InetAddress>::iterator listEnd = candidates->end();
                for(std::list<InetAddress>::iterator it = listBegin; it != listEnd; ++it)
                {
                    if(printSteps)
                        (*out) << (*it) << "\n";
                    
                    /*
                     * IPs besides the first candidate are inserted as well with right TTL, 
                     * because it is possible the subnet actually features back-up interface(s) 
                     * which are still relevant in subsequent steps (like alias resolution).
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
                
                if(printSteps)
                {
                    (*out) << "Final result is " << lowerBorder << "/" << (unsigned short) prefix;
                    (*out) << " (odd subnet).\n" << endl;
                }
                else
                {
                    (*out) << "Successfully expanded into " << lowerBorder << "/";
                    (*out) << (unsigned short) prefix << " (odd subnet)." << endl;
                }
                
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
    
    if(printSteps)
    {
        (*out) << "Cannot find a valid contra-pivot interface even if a /20 is considered.\n";
        (*out) << initLowerBorder << "/" << (unsigned short) initialPrefix << " will be marked ";
        (*out) << "as shadow subnet.\n" << endl;
    }
    else
    {
        (*out) << "It could not be successfully expanded (hit the /20 limit) and will be marked ";
        (*out) << "as a shadow subnet." << endl;
    }
    
    ss->setStatus(SubnetSite::SHADOW_SUBNET);
    
    // Creates equivalent /20 subnet in "toAvoid" set
    if(ss->getPivotAddress() != InetAddress(0))
    {
        InetAddress pivotIP(ss->getPivotAddress());
    
        SubnetSiteSet *zonesToAvoid = env->getIPBlocksToAvoid();
        SubnetSite *zoneToAvoid = new SubnetSite();
        zoneToAvoid->setPivotAddress(pivotIP);
        zoneToAvoid->setInferredSubnetPrefixLength((unsigned char) 20);
        zoneToAvoid->setStatus(SubnetSite::UNDEFINED_SUBNET);
        zoneToAvoid->setShortestTTL(reqTTL);
        zoneToAvoid->setGreatestTTL(reqTTL);
        zoneToAvoid->insert(new SubnetSiteNode(pivotIP, 
                                               (unsigned char) 20, 
                                               reqTTL, 
                                               SubnetSiteNode::UNKNOWN_ALIAS_SX));
        
        zonesToAvoid->getSubnetSiteList()->push_back(zoneToAvoid);
    }
}

void SubnetRefiner::fill(SubnetSite *ss)
{
    if(ss == NULL)
       return;

    // Relevant pointers from env
    ostream *out = env->getOutputStream();
    IPLookUpTable *table = env->getIPTable();
    
    // Small message in console (only if user asks to print steps)
    if(printSteps)
    {
        (*out) << "Filling " << ss->getInferredNetworkAddressString() << "... ";
    }
    
    InetAddress lowerBorder, upperBorder;
    std::list<SubnetSiteNode*> *IPsList = ss->getSubnetIPList();
    unsigned char prefixLength = ss->getInferredSubnetPrefixLength();
    if(prefixLength > 32)
    {
        if(printSteps)
        {
            (*out) << "Some weird stuff happened (prefix length > 32 ?!)." << endl;
        }
        return;
    }
    
    // For some reason, this occured on PlanetLab... by security, we stop here.
    if(prefixLength == 0)
    {
        if(printSteps)
        {
            (*out) << "Some weird stuff happened (prefix length = 0 ?!)." << endl;
        }
        return;
    }
    
    // Checks if we dont have already the maximum number of elements
    unsigned int maxSize = (unsigned int) pow(2.0, (double) (32 - (unsigned int) prefixLength));
    if(maxSize == IPsList->size())
    {
        if(printSteps)
        {
            (*out) << "All possible interfaces were already listed." << endl;
        }
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
    
    // Goes through the list to determine which IP is already known
    for(list<SubnetSiteNode*>::iterator it = IPsList->begin(); it != IPsList->end(); ++it)
    {
        SubnetSiteNode *cur = (*it);
        if(cur == NULL) // By security
            continue;
        
        unsigned long offset = cur->ip.getULongAddress() - lowerBorder.getULongAddress();
        if(offset >= 0 && offset < maxSize) // Security
            knownIPs[offset] = 1;
    }
    
    // Inserts in the subnet IPs found in the IP look-up table but absent from the list
    unsigned short newIPs = 0;
    for(InetAddress i = lowerBorder; i <= upperBorder; i++)
    {
        unsigned long offset = i.getULongAddress() - lowerBorder.getULongAddress();
        if(offset < 0 || offset >= maxSize) // Security
            continue;
        
        if(knownIPs[offset] == 0)
        {
            IPTableEntry *iEntry = table->lookUp(i);
            
            /*
             * Just in case, it is also checked that the IP found in the dictionnary also has a 
             * TTL value which is different from the default value (= no "good" TTL computed for 
             * this IP) though this should ideally not occur.
             */
            
            if(iEntry != NULL && iEntry->getTTL() != IPTableEntry::NO_KNOWN_TTL)
            {
                ss->insert(new SubnetSiteNode(i, 
                                              prefixLength, 
                                              iEntry->getTTL(), 
                                              SubnetSiteNode::UNKNOWN_ALIAS_SX));
                
                newIPs++;
            }
        }
    }
    
    // Sorts the final list if new IPs were added + message stating the amount of added IPs
    if(newIPs > 0)
    {
        IPsList->sort(SubnetSiteNode::smaller);
        if(printSteps)
        {
            if(newIPs > 1)
                (*out) << newIPs << " missing responsive interfaces have been added." << endl;
            else
                (*out) << "One missing responsive interface has been added." << endl;
        }
    }
    else
    {
        if(printSteps)
        {
            (*out) << "All responsive interfaces were already listed." << endl;
        }
    }
}

void SubnetRefiner::shadowExpand(SubnetSite *ss)
{
    // Relevant pointers from env
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnetSet = env->getSubnetSet();

    // Starts printing out
    if(printSteps)
    {
        (*out) << "Expanding shadow subnet " << ss->getInferredNetworkAddressString() << "..." << endl;
    }

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
    InetAddress previousLowerBorder = initLowerBorder;
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
        
        // Checks compatibility
        if(!subnetSet->isCompatible(lowerBorder, 
                                    upperBorder, 
                                    ss->getShortestTTL(), 
                                    beforeAndAfter,
                                    true))
        {
            if(printSteps)
            {
                (*out) << lowerBorder << "/" << (unsigned short) prefix;
                (*out) << " collides with other inferred subnets.\n";
                (*out) << previousLowerBorder << "/" << (unsigned short) (prefix + 1);
                (*out) << " is the best upper bound for this shadow subnet.\n" << endl;
            }
            
            ss->setInferredSubnetPrefixLength(prefix + 1);
            return;
        }
        
        if(printSteps)
        {
            (*out) << lowerBorder << "/" << (unsigned short) prefix << " is possible." << endl;
        }
        
        previousLowerBorder = lowerBorder;
        prefix--;
    }
    
    unsigned char finalPrefix = SubnetRefiner::LOWEST_PREFIX_ALLOWED;
    
    if(printSteps)
    {
        (*out) << "This subnet reached its maximum possible size (/";
        (*out) << (unsigned short) finalPrefix << ").\n" << endl;
    }
    
    ss->setInferredSubnetPrefixLength(finalPrefix);
}
