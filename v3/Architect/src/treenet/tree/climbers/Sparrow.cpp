/*
 * Sparrow.cpp
 *
 *  Created on: Aug 22, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Sparrow.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Sparrow.h"

Sparrow::Sparrow(TreeNETEnvironment *env) : Climber(env)
{
}

Sparrow::~Sparrow()
{
}

void Sparrow::climb(Soil *fromSoil)
{
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    if(roots->size() > 1)
    {
        unsigned short treeIndex = 1;
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            this->climbRecursive((*i)->getRoot(), 0);
            treeIndex++;
        }
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot(), 0);
    }
}

void Sparrow::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
{
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
        list<Aggregate*> *aggs = cur->getAggregates();
        
        // Builds the aggregates for multi-label nodes, taking pre-aliasing into account
        if(cur->isHedera())
        {
            IPLookUpTable *dict = env->getIPTable();
            list<InetAddress> *labels = cur->getLabels();
            list<InetAddress> known; // Prevents duplicata
            for(list<InetAddress>::iterator it = labels->begin(); it != labels->end(); ++it)
            {
                if((*it) == InetAddress(0))
                {
                    aggs->push_back(new Aggregate((*it)));
                    continue;
                }
            
                // Check if (*it) is not already known
                bool isKnown = false;
                for(list<InetAddress>::iterator i = known.begin(); i != known.end(); ++i)
                {
                    if((*i) == (*it))
                    {
                        isKnown = true;
                        break;
                    }
                }
                if(isKnown)
                    continue;
                
                IPTableEntry *ip = dict->lookUp((*it));
                if(ip == NULL)
                    continue;
                
                // No pre-alias: creates an aggregate just for this IP
                list<InetAddress> *pres = ip->getPreAliases();
                if(pres->size() == 0)
                {
                    aggs->push_back(new Aggregate((*it)));
                    continue;
                }
                
                list<InetAddress> aggregateIPs;
                aggregateIPs.push_back((*it));
                for(list<InetAddress>::iterator i = pres->begin(); i != pres->end(); ++i)
                {
                    aggregateIPs.push_back((*i));
                    known.push_back((*i));
                }
                
                aggs->push_back(new Aggregate(aggregateIPs));
            }
            cur->buildAggregates();
        }
        // Simple internal: only one aggregate
        else
        {
            aggs->push_back(new Aggregate(cur->getLabels()->front()));
            cur->buildAggregates();
        }
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i), depth + 1);
        }
    }
}
