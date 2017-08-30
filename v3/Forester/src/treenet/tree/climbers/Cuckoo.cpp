/*
 * Cuckoo.cpp
 *
 *  Created on: Sept 13, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Cuckoo.h (see this file to learn further about the goals of 
 * such class).
 */

#include "../../../common/thread/Thread.h" // For invokeSleep()
#include "Cuckoo.h"

Cuckoo::Cuckoo(TreeNETEnvironment *env) : Climber(env)
{
    ahc = new AliasHintCollector(env);
}

Cuckoo::~Cuckoo()
{
    delete ahc;
}

void Cuckoo::climb(Soil *fromSoil)
{
    // Small delay before starting with the first internal (typically half a second)
    Thread::invokeSleep(env->getProbeThreadDelay() * 2);
    
    ostream *out = env->getOutputStream();
    list<NetworkTree*> *roots = fromSoil->getRootsList();

    if(roots->size() > 1)
    {
        unsigned short treeIndex = 1;
        
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            (*out) << "[Tree n°" << treeIndex << "]\n" << endl;
            this->climbRecursive((*i)->getRoot(), 0);
            (*out) << endl;
            
            treeIndex++;
        }
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot(), 0);
        (*out) << endl;
    }
}

void Cuckoo::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
{
    ostream *out = env->getOutputStream();
    
    // Root: goes deeper
    if(cur->isRoot())
    {
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i), depth + 1);
        }
    }
    // Leaf: stops
    else if(cur->isLeaf())
    {
        return;
    }
    // Any other case: internal node
    else
    {
        if(cur->countInterfaces() > 1)
        {
            (*out) << "Collecting alias resolution hints for ";
            if(cur->isHedera())
            {
                (*out) << "Hedera {";
                list<InetAddress> *labels = cur->getLabels();
                bool guardian = false;
                for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                {
                    if (guardian)
                        (*out) << ", ";
                    else
                        guardian = true;
                
                    (*out) << (*i);
                }
                (*out) << "}..." << endl;
            }
            else
            {
                (*out) << "Neighborhood {" << cur->getLabels()->front() << "}... " << std::flush;
            }
            
            if(this->ahc->isPrintingSteps())
            {
                (*out) << endl;
                if(this->ahc->debugMode()) // Additionnal line break for harmonious display
                    (*out) << endl;
            }
            
            /*
             * Added in February 2017: now, the hint collection for multi-label nodes (or 
             * Hedera's) is scheduled such that alias candidates which share the same last hop in 
             * the route to their respective subnet are probed together. A small delay is also 
             * introduced before the next set to avoid an aggressive probing. The idea is 
             * motivated by the fact that early hedera's in a tree sometimes gather an extensive 
             * amount of subnets below them (sometimes, it is even worse because of anonymous 
             * hops). This causes the alias resolution by IP-ID, especially the velocity-based 
             * method, to produce too optimistic results.
             */
            
            if(cur->isHedera())
            {
                list<Aggregate*> *aggs = cur->getAggregates();
                for(list<Aggregate*>::iterator it = aggs->begin(); it != aggs->end(); ++it)
                {
                    Aggregate *curAgg = (*it);
                    InetAddress lastHop = curAgg->getFirstLastHop();
                    
                    (*out) << "Alias candidates which last hop is " << lastHop << "... " << std::flush;
                    if(this->ahc->isPrintingSteps()) // Additionnal line break for better output
                        (*out) << endl;
                    
                    this->ahc->setIPsToProbe(curAgg->listAllInterfaces());
                    try
                    {
                        this->ahc->collect();
                    }
                    catch(StopException e)
                    {
                        throw;
                    }
                    
                    Thread::invokeSleep(env->getProbeThreadDelay());
                }
            }
            // Simple internal: only one collection
            else
            {
                Aggregate *agg = cur->getFirstAggregate();
                if(agg != NULL)
                {
                    this->ahc->setIPsToProbe(agg->listAllInterfaces());
                    try
                    {
                        this->ahc->collect();
                    }
                    catch(StopException e)
                    {
                        throw;
                    }
                }
            }
            
            // Small delay before analyzing next internal (typically quarter of a second)
            Thread::invokeSleep(env->getProbeThreadDelay());
        }
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i), depth + 1);
        }
    }
}
