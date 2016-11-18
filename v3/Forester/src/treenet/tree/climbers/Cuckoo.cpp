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
        list<InetAddress> interfacesToProbe = cur->listInterfaces();
        
        if(interfacesToProbe.size() > 1)
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
                (*out) << "}";
            }
            else
            {
                (*out) << "Neighborhood {" << cur->getLabels()->front() << "}";
            }
            
            (*out) << "... " << std::flush;
            
            if(this->ahc->isPrintingSteps())
            {
                (*out) << endl;
                if(this->ahc->debugMode()) // Additionnal line break for harmonious display
                    (*out) << endl;
            }
            
            this->ahc->setIPsToProbe(interfacesToProbe);
            this->ahc->setCurrentTTL((unsigned char) depth);
            try
            {
                this->ahc->collect();
            }
            catch(StopException e)
            {
                throw;
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
