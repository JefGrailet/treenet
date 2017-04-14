/*
 * Termite.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Termite.h (see this file to learn further about the goals of 
 * such class).
 */

#include <iomanip>
using std::setprecision;

#include "Termite.h"

Termite::Termite(TreeNETEnvironment *env) : Climber(env)
{
}

Termite::~Termite()
{
}

void Termite::climb(Soil *fromSoil)
{
    ostream *out = env->getOutputStream();
    list<NetworkTree*> *roots = fromSoil->getRootsList();

    if(roots->size() > 1)
    {
        unsigned short treeIndex = 1;
        
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            (*out) << "[Tree n°" << treeIndex << "]\n" << endl;
            this->climbRecursive((*i)->getRoot());
            (*out) << endl;
            
            treeIndex++;
        }
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot());
        (*out) << endl;
    }
}

void Termite::evaluate(list<Router*> routers)
{
    ostream *out = env->getOutputStream();

    unsigned short reqNbInterfaces = (unsigned short) routers.size() - 1;
    unsigned short k = 0;
    float sumProba = 0.0;
    for(list<Router*>::iterator i = routers.begin(); i != routers.end(); ++i)
    {
        unsigned short nbInterfaces = (*i)->getNbInterfaces();
        float proba = (float) nbInterfaces / (float) (nbInterfaces + reqNbInterfaces);
        proba = 1.0 - proba;
        sumProba += proba;
        k++;
    
        (*out) << (*i)->toStringMinimalist() << " - " << setprecision(3) << proba * 100;
        (*out) << "%" << endl;
    }
    
    float avg = sumProba / (float) routers.size();
    (*out) << "\nAverage probability: " << setprecision(3) << avg * 100 << "%\n" << endl;
}

void Termite::climbRecursive(NetworkTreeNode *cur)
{
    ostream *out = env->getOutputStream();
    
    // Root: goes deeper
    if(cur->isRoot())
    {
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i));
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
        list<Router*> *routers = cur->getInferredRouters();
        if(routers->size() == 0)
        {
            // Goes deeper and quits
            list<NetworkTreeNode*> *children = cur->getChildren();
            for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
            {
                this->climbRecursive((*i));
            }
            return;
        }
        
        list<InetAddress> *labels = cur->getLabels();
        if(cur->isRoot())
        {
            (*out) << "Root neighborhood\n";
        }
        else
        {
            if(cur->isHedera())
            {
                (*out) << "Neighborhood/Hedera {";
                bool guardian = false;
                for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                {
                    if (guardian)
                        (*out) << ", ";
                    else
                        guardian = true;
                
                    (*out) << (*i);
                }
                (*out) << "}\n";
            }
            else
            {
                (*out) << "Neighborhood {" << labels->front() << "}\n";
            }
        }
    
        if(cur->isHedera())
        {
            list<InetAddress> lastHops;
            list<list<InetAddress> > sets = cur->listInterfacesByLastHop(&lastHops);
            
            while(sets.size() > 0)
            {
                list<InetAddress> curGroup = sets.front();
                InetAddress curLastHop = lastHops.front();
                
                // Lists routers which contain the IPs from "curGroup"
                list<Router*> curRouters;
                for(list<InetAddress>::iterator i = curGroup.begin(); i != curGroup.end(); ++i)
                {
                    InetAddress curIP = (*i);
                    for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                    {
                        Router *curRouter = (*j);
                        if(curRouter->hasInterface(curIP))
                        {
                            curRouters.push_back(curRouter);
                            
                            // Removes interfaces of curRouter from "curGroup" list
                            list<InetAddress>::iterator start = i;
                            for(list<InetAddress>::iterator k = ++start; k != curGroup.end(); ++k)
                            {
                                if(curRouter->hasInterface((*k)))
                                    curGroup.erase(k--);
                            }
                            curGroup.erase(i--);
                            break;
                        }
                    }
                }
                
                if(curRouters.size() > 0)
                {
                    (*out) << "Last hop = " << curLastHop << ":" << endl;
                    this->evaluate(curRouters);
                }
                
                lastHops.pop_front();
                sets.pop_front();
            }
        }
        else
        {
            this->evaluate(*routers);
        }
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i));
        }
    }
}
