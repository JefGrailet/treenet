/*
 * AnonymousChecker.cpp
 *
 *  Created on: Feb 15, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in AnonymousChecker.h (see this file to learn further about the 
 * goals of such class).
 */

#include "AnonymousChecker.h"
#include "AnonymousCheckUnit.h"
#include "../../../../common/thread/Thread.h"

AnonymousChecker::AnonymousChecker(TreeNETEnvironment *env)
{
    this->env = env;
    this->totalAnonymous = 0;
    this->totalSolved = 0;
    
    this->loadTargets();
}

AnonymousChecker::~AnonymousChecker()
{
}

unsigned int AnonymousChecker::getTotalFullyRepaired()
{
    unsigned int total = 0;
    list<SubnetSite*> toIterate = this->incompleteSubnets;
    for(list<SubnetSite*>::iterator it = toIterate.begin(); it != toIterate.end(); ++it)
        if((*it)->hasCompleteRoute())
            total++;
    return total;
}

float AnonymousChecker::getRatioSolvedHops()
{
    return (float) this->totalSolved / (float) this->totalAnonymous;
}

void AnonymousChecker::reload()
{
    this->totalAnonymous = 0;
    this->totalSolved = 0;
    this->incompleteSubnets.clear();
    this->targetSubnets.clear();
    this->toFixOffline.clear();
    
    this->loadTargets();
}

void AnonymousChecker::callback(SubnetSite *target, unsigned short hop, InetAddress solution)
{
    RouteInterface *route = target->getRoute();
    route[hop].deanonymize(solution);
    this->totalSolved++;
    
    if(hop == target->getRouteSize() - 1)
        return; // Stop here, there won't be any offline fix with this hop
    
    map<SubnetSite*, list<SubnetSite*> >::iterator lookUp = toFixOffline.find(target);
    if(lookUp == toFixOffline.end())
        return;
    
    list<SubnetSite*> routesToFix = lookUp->second;
    if(routesToFix.size() == 0)
        return;
    
    for(list<SubnetSite*>::iterator it = routesToFix.begin(); it != routesToFix.end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        
        unsigned short curRouteSize = curSubnet->getRouteSize();
        RouteInterface *curRoute = curSubnet->getRoute();
        
        if(hop >= (curRouteSize - 1) || hop == 0)
            continue;
        
        if(route[hop - 1].ip != curRoute[hop - 1].ip || route[hop + 1].ip != curRoute[hop + 1].ip)
            continue;
        
        if(curRoute[hop].ip != InetAddress(0)) // Just in case
            continue;
        
        curRoute[hop].repairBis(solution);
        this->totalSolved++;
    }
}

void AnonymousChecker::probe()
{
    unsigned short maxThreads = env->getMaxThreads();
    unsigned int nbTargets = this->totalAnonymous;
    
    if(maxThreads > MAX_THREADS)
        maxThreads = MAX_THREADS;
    
    if(nbTargets == 0)
        return;
    
    // N.B.: ideally, we want this phase to last approx. one hour.
    unsigned short nbThreads = MIN_THREADS;
    unsigned int targetsPerThread = THREAD_PROBES_PER_HOUR;
    
    if(nbTargets > targetsPerThread && (nbTargets / targetsPerThread) > MIN_THREADS)
        nbThreads = (unsigned short) (nbTargets / targetsPerThread);
    
    if(nbThreads > maxThreads)
        nbThreads = maxThreads;
    
    targetsPerThread = nbTargets / (unsigned int) nbThreads;
    if(targetsPerThread == 0)
        targetsPerThread = 1;
    
    // Computes the list of targets now
    list<list<SubnetSite*> > targets;
    while(this->targetSubnets.size() > 0)
    {
        list<SubnetSite*> curList;
        unsigned int curNbTargets = 0;
        while(curNbTargets < targetsPerThread && this->targetSubnets.size() > 0)
        {
            SubnetSite *front = this->targetSubnets.front();
            this->targetSubnets.pop_front();
            
            curNbTargets += front->countMissingHops();
            curList.push_back(front);
        }
        
        if(curList.size() > 0)
            targets.push_back(curList);
    }
    
    unsigned short trueNbThreads = (unsigned short) targets.size();
    
    // Prepares and launches threads
    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / trueNbThreads;
    Thread **th = new Thread*[trueNbThreads];
    
    for(unsigned short i = 0; i < trueNbThreads; i++)
    {
        list<SubnetSite*> targetsSubset = targets.front();
        targets.pop_front();

        Runnable *task = NULL;
        try
        {
            task = new AnonymousCheckUnit(env, 
                                          this, 
                                          targetsSubset, 
                                          DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                          DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                          DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                          DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

            th[i] = new Thread(task);
        }
        catch(SocketException &se)
        {
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < trueNbThreads; k++)
            {
                delete th[k];
                th[k] = NULL;
            }
            
            delete[] th;
            
            throw StopException();
        }
        catch(ThreadException &te)
        {
            ostream *out = env->getOutputStream();
            (*out) << "Unable to create more threads." << endl;
                
            delete task;
        
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < trueNbThreads; k++)
            {
                delete th[k];
                th[k] = NULL;
            }
            
            delete[] th;
            
            throw StopException();
        }
    }

    // Launches thread(s) then waits for completion
    for(unsigned int i = 0; i < trueNbThreads; i++)
    {
        th[i]->start();
        Thread::invokeSleep(env->getProbeThreadDelay());
    }
    
    for(unsigned int i = 0; i < trueNbThreads; i++)
    {
        th[i]->join();
        delete th[i];
    }
    
    delete[] th;
    
    // Might happen because of SocketSendException thrown within a unit
    if(env->isStopping())
    {
        throw StopException();
    }
}

// Implementation of private methods.

bool AnonymousChecker::similarAnonymousHops(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned short sizeRouteSs1 = ss1->getRouteSize();
    unsigned short sizeRouteSs2 = ss2->getRouteSize();
    RouteInterface *routeSs1 = ss1->getRoute();
    RouteInterface *routeSs2 = ss2->getRoute();
    for(unsigned short i = 1; i < sizeRouteSs1 - 1; i++) // We don't fix offline first/last hop (risky)
    {
        if(i >= sizeRouteSs2 - 1)
            break;
        
        if(routeSs1[i].ip == InetAddress(0))
        {
            InetAddress hopBefore = routeSs1[i - 1].ip;
            InetAddress hopAfter = routeSs1[i + 1].ip;
            
            if(routeSs2[i - 1].ip == hopBefore && routeSs2[i + 1].ip == hopAfter)
                return true;
        }
    }
    return false;
}

void AnonymousChecker::loadTargets()
{
    // Lists subnets with incomplete routes
    list<SubnetSite*> sparseRoutes;
    list<SubnetSite*> *fullList = this->env->getSubnetSet()->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = fullList->begin(); it != fullList->end(); it++)
    {
        SubnetSite *cur = (*it);
        if(cur->hasValidRoute() && cur->hasIncompleteRoute())
        {
            sparseRoutes.push_back(cur);
            this->incompleteSubnets.push_back(cur);
            this->totalAnonymous += cur->countMissingHops();
        }
    }
    
    for(list<SubnetSite*>::iterator it = sparseRoutes.begin(); it != sparseRoutes.end(); ++it)
    {
        SubnetSite *cur = (*it);
        this->targetSubnets.push_back(cur);
        
        // Lists subnets which have similar missing steps
        list<SubnetSite*> similar;
        list<SubnetSite*>::iterator start = it;
        for(list<SubnetSite*>::iterator it2 = ++start; it2 != sparseRoutes.end(); ++it2)
        {
            SubnetSite *cur2 = (*it2);
            if(similarAnonymousHops(cur, cur2))
            {
                similar.push_back(cur2);
                sparseRoutes.erase(it2--);
            }
        }
        
        this->toFixOffline.insert(pair<SubnetSite*, list<SubnetSite*> >(cur, similar));
    }
}
