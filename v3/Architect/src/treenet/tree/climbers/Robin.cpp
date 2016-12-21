/*
 * Robin.cpp
 *
 *  Created on: Sept 13, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Robin.h (see this file to learn further about the goals of such 
 * class).
 */

#include <list>
using std::list;

#include "Robin.h"

Robin::Robin(TreeNETEnvironment *env) : Climber(env)
{
    // Nothing special here
}

Robin::~Robin()
{
}

void Robin::climb(Soil *fromSoil)
{
    ostream *out = env->getOutputStream();
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    
    if(roots->size() > 1)
    {
        unsigned short treeIndex = 1;
        
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            (*out) << "Tree n°" << treeIndex << endl;
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

void Robin::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
{
    ostream *out = env->getOutputStream();
    
    // Prints current depth
    (*out) << depth << " - ";
    
    // Displays root node
    if(cur->isRoot())
    {
        (*out) << "Root node" << endl;
        
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i), depth + 1);
        }
    }
    // Displays a leaf (subnet)
    else if(cur->isLeaf())
    {
        (*out) << "Subnet: " << cur->getAssociatedSubnet()->getInferredNetworkAddressString() << endl;
    }
    // Any other case: internal node (with or without load balancing)
    else
    {
        if(cur->isHedera())
        {
            (*out) << "Internal - Hedera: ";
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
        }
        else
        {
            (*out) << "Internal - Neighborhood: " << cur->getLabels()->front();
        }
        
        // Shows previous label(s) appearing in the routes of subnets of this branch
        list<InetAddress> *prevLabels = cur->getPreviousLabels();
        if(prevLabels->size() > 0)
        {
            (*out) << " (Previous: ";
            bool guardian = false;
            for(list<InetAddress>::iterator i = prevLabels->begin(); i != prevLabels->end(); ++i)
            {
                if (guardian)
                    (*out) << ", ";
                else
                    guardian = true;
            
                (*out) << (*i);
            }
            (*out) << ")";
        }
        
        (*out) << endl;
        
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i), depth + 1);
        }
    }
}
