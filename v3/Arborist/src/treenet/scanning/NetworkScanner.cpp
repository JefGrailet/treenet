/*
 * NetworkScanner.cpp
 *
 *  Created on: Nov 14, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in NetworkScanner.h (see this file to learn further about the 
 * goals of such class).
 */

#include "NetworkScanner.h"
#include "./explorenet/ExploreNETRunnable.h"
#include "../../common/thread/Thread.h"

NetworkScanner::NetworkScanner(TreeNETEnvironment *env)
{
    this->env = env;
    this->sr = new SubnetRefiner(this->env);
}

NetworkScanner::~NetworkScanner()
{
    delete sr;
}

void NetworkScanner::scan(list<InetAddress> targets)
{
    ostream *out = env->getOutputStream();
    unsigned short displayMode = env->getDisplayMode();
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    SubnetSiteSet *zonesToAvoid = env->getIPBlocksToAvoid();

    // Size of threads vector
    unsigned short nbThreads = env->getMaxThreads();
    unsigned long nbTargets = (unsigned int) targets.size();
    unsigned short sizeArray = (unsigned short) nbTargets;
    if(nbTargets > (unsigned long) nbThreads)
        sizeArray = nbThreads;
    
    // Creates thread(s)
    Thread **th = new Thread*[sizeArray];
    for(unsigned short i = 0; i < sizeArray; i++)
        th[i] = NULL;

    (*out) << "Starting network scanning..." << endl;
    if(!(displayMode == TreeNETEnvironment::DISPLAY_MODE_DEBUG || displayMode == TreeNETEnvironment::DISPLAY_MODE_VERBOSE))
        (*out) << endl;
    
    while(nbTargets > 0)
    {
        unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
        range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
        range /= sizeArray;
        
        for(unsigned short i = 0; i < sizeArray; i++)
        {
            InetAddress curTarget(targets.front());
            targets.pop_front();
            
            unsigned short lowerBound = (i * range);
            unsigned short upperBound = lowerBound + range - 1;
            
            Runnable *task = NULL;
            try
            {
                task = new ExploreNETRunnable(env, 
                                              curTarget, 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowerBound, 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upperBound, 
                                              DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                              DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
                
                th[i] = new Thread(task);
            }
            catch(SocketException &se)
            {
                for(unsigned short j = 0; j < sizeArray; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                throw StopException();
            }
            catch(ThreadException &te)
            {
                delete task;
            
                for(unsigned short j = 0; j < sizeArray; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                throw StopException();
            }
        }

        // Launches thread(s) then waits for completion
        for(unsigned short i = 0; i < sizeArray; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->start(); // N.B.: each thread will write in the console
                Thread::invokeSleep(env->getProbeThreadDelay());
            }
        }
        
        for(unsigned short i = 0; i < sizeArray; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->join();
                delete th[i];
                th[i] = NULL;
            }
        }
        
        (*out) << endl;
        
        if(env->isStopping())
        {
            delete[] th;
            throw StopException();
        }
        
        /*
         * BYPASS
         *
         * After probing a certain amount of target addresses, TreeNET periodically 
         * stops the scanning in order to perform a first form of refinement: expansion.
         * Indeed, ExploreNET tends to partition subnets when they do not feature a 
         * certain amount of responsive interfaces. Expansion aims at correcting this 
         * issue by relying on the Contra-Pivot notion (see SubnetRefiner.cpp/.h for more 
         * details about this).
         *
         * Because expansion overgrowths subnet, performing this refinement may avoid 
         * probing several addresses that are already inside the boundaries of the 
         * refined subnets. Therefore, this should speed up the scanning which is the 
         * slowest operation of TreeNET.
         */
        
        // We first check which subnet should be refined
        list<SubnetSite*> *ssList = subnetSet->getSubnetSiteList();
        bool needsRefinement = false;
        string newSubnets = "";
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            if((*it)->getStatus() != SubnetSite::NOT_PREPARED_YET)
                continue;
            
            (*it)->prepareForRefinement();
            string networkAddressStr = (*it)->getInferredNetworkAddressString();
            switch((*it)->getStatus())
            {
                case SubnetSite::INCOMPLETE_SUBNET:
                    needsRefinement = true;
                    newSubnets += networkAddressStr + ": incomplete subnet\n";
                    break;
                case SubnetSite::ACCURATE_SUBNET:
                    newSubnets += networkAddressStr + ": accurate subnet\n";
                    break;
                case SubnetSite::ODD_SUBNET:
                    newSubnets += networkAddressStr + ": odd subnet\n";
                    break;
                default:
                    newSubnets += networkAddressStr + ": undefined subnet\n";
                    break;
            }
        }
        
        if(!newSubnets.empty())
        {
            (*out) << "New subnets found by the previous " << sizeArray << " threads:\n";
            (*out) << newSubnets << endl;
        }
        else
        {
            (*out) << "Previous " << sizeArray << " threads found no new subnet\n" << endl;
        }
        
        // Performs refinement
        if(needsRefinement)
        {
            (*out) << "Refining incomplete subnets...\n" << endl;
            
            SubnetSite *candidateForRefinement = subnetSet->getIncompleteSubnet();
            while(candidateForRefinement != NULL)
            {
                /*
                 * Checks if expansion should be conducted, i.e. the subnet should not be 
                 * encompassed by an UNDEFINED subnet found in the "IPBlocksToAvoid" set from 
                 * TreeNETEnvironment. Otherwise, expansion is a waste of time: the presence 
                 * of an UNDEFINED subnet means we already looked for Contra-Pivot interfaces 
                 * in this zone without success. There should not be any issue regarding TTL 
                 * values, because expansion should have stopped before reaching the UNDEFINED 
                 * state in this case. When refinement by expansion is not done, the subnet is 
                 * directly labelled as SHADOW and reinserted in subnetSet.
                 */
                
                SubnetSite *ss2 = zonesToAvoid->isSubnetEncompassed(candidateForRefinement);
                if(ss2 != NULL)
                {
                    string ssStr = candidateForRefinement->getInferredNetworkAddressString();
                    string ss2Str = ss2->getInferredNetworkAddressString();
                    (*out) << "No refinement for " << ssStr << ": it is encompassed in the ";
                    (*out) << ss2Str << " IPv4 address block.\nThis block features Pivot ";
                    (*out) << "interfaces with the same TTL as expected for " << ssStr << ".\n";
                    (*out) << "It has already been checked to find Contra-Pivot interfaces, ";
                    (*out) << "without success.\n";
                    (*out) << ssStr << " marked as SHADOW subnet.\n" << endl;
                
                    candidateForRefinement->setStatus(SubnetSite::SHADOW_SUBNET);
                    
                    // No check of return value because subnet did not change
                    subnetSet->addSite(candidateForRefinement); 
                    
                    candidateForRefinement = subnetSet->getIncompleteSubnet();
                    continue;
                }
                
                try
                {
                    sr->expand(candidateForRefinement);
                }
                catch(StopException &se)
                {
                    // Deletes remaining resources involved in scanning and re-throws
                    delete candidateForRefinement;
                    delete[] th;
                    
                    throw;
                }
                
                unsigned short res = subnetSet->addSite(candidateForRefinement);
                
                if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
                {
                    delete candidateForRefinement;
                }
            
                candidateForRefinement = subnetSet->getIncompleteSubnet();
            }
            
            /*
             * If we are in laconic display mode, we add a line break before the next message 
             * to keep the display pretty to read.
             */
            
            if(displayMode == TreeNETEnvironment::DISPLAY_MODE_LACONIC)
                (*out) << "\n";
            
            (*out) << "Back to scanning...\n" << endl;
        }
        
        // Updates number of targets for next salve of threads
        if(nbTargets > (unsigned long) sizeArray)
        {
            nbTargets -= (unsigned long) sizeArray;
            if(nbTargets < (unsigned long) sizeArray)
                sizeArray = (unsigned short) nbTargets;
        }
        else
            nbTargets = 0;
        
        if(env->isStopping())
        {
            break;
        }
    }
    
    delete[] th;
    
    (*out) << "Scanning completed.\n" << endl;
}

void NetworkScanner::finalize()
{
    ostream *out = env->getOutputStream();
    unsigned short displayMode = env->getDisplayMode();
    SubnetSiteSet *subnetSet = env->getSubnetSet();

    int nbShadows = 0;
    list<SubnetSite*> *ssList = subnetSet->getSubnetSiteList();
    list<SubnetSite*> toFill;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        // Security for PlanetLab version (though this occurs rarely)
        if((*it) == NULL)
            continue;
    
        unsigned short status = (*it)->getStatus();
        if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
            toFill.push_back((*it));
        else if((*it)->getStatus() == SubnetSite::SHADOW_SUBNET)
            nbShadows++;
    }
    
    // Filling
    if(toFill.size() > 0)
    {
        (*out) << "Refinement by filling (passive method)...\n" << endl;
        
        for(list<SubnetSite*>::iterator it = toFill.begin(); it != toFill.end(); ++it)
        {
            // Security for PlanetLab version (though this should not occur)
            if((*it) == NULL)
                continue;
            
            unsigned short status = (*it)->getStatus();
            
            sr->fill(*it);
            // (*it) != NULL: same reason as above (though, once again, unlikely)
            if((*it) != NULL && status == SubnetSite::ACCURATE_SUBNET)
                (*it)->recomputeRefinementStatus();
        }
        
        /*
         * Like at the end of Bypass round, we have to take into account the display mode 
         * before adding a new line break to space harmoniously the different steps.
         */
        
        if(displayMode != TreeNETEnvironment::DISPLAY_MODE_LACONIC)
            (*out) << endl;
    }
    
    // Shadow expansion (no parallelization here, because this operation is instantaneous)
    if(nbShadows > 0)
    {
        (*out) << "Expanding shadow subnets to the maximum...\n" << endl;
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            if((*it)->getStatus() == SubnetSite::SHADOW_SUBNET)
            {
                sr->shadowExpand(*it);
            }
        }
        
        /*
         * Removes all shadow subnets, then puts them back. The motivation is to merge the subnets 
         * that have the same prefix length after shadow expansion, because it is rather frequent 
         * that several incomplete subnets lead to the same shadow subnet.
         *
         * It can also occur that a shadow subnet actually contains one or several outliers 
         * from another subnet. In that case, it should be merged with the larger subnet.
         */
        
        SubnetSite *shadow = subnetSet->getShadowSubnet();
        list<SubnetSite*> listShadows;
        while(shadow != NULL)
        {
            listShadows.push_back(shadow);
            shadow = subnetSet->getShadowSubnet();
        }
        
        list<SubnetSite*>::iterator listBegin = listShadows.begin();
        list<SubnetSite*>::iterator listEnd = listShadows.end();
        for(list<SubnetSite*>::iterator it = listBegin; it != listEnd; ++it)
        {
            unsigned short res = subnetSet->addSite((*it));
            if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
            {
                delete (*it);
            }
        }
    }
}
