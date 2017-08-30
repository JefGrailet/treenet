/*
 * Magie.cpp
 *
 *  Created on: Aug 23, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Magpie.h (see this file to learn further about the goals of 
 * such class).
 */

#include "../../../common/thread/Thread.h" // For invokeSleep()
#include "Magpie.h"

Magpie::Magpie(TreeNETEnvironment *env) : Climber(env)
{
    ahc = new AliasHintCollector(env);
    ar = new AliasResolver(env);
}

Magpie::~Magpie()
{
    delete ahc;
    delete ar;
}

void Magpie::climb(Soil *fromSoil)
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

void Magpie::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
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
        if(cur->isHedera())
        {
            (*out) << "Collecting pre-alias resolution hints on Hedera {";
            list<InetAddress> *labels = cur->getLabels();
            list<InetAddress> ahcTargets;
            bool guardian = false;
            for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
            {
                InetAddress curLabel = (*i);
                if (guardian)
                    (*out) << ", ";
                else
                    guardian = true;
            
                (*out) << curLabel;
                ahcTargets.push_back(InetAddress(curLabel));
            }
            (*out) << "}... " << std::flush;
            
            if(this->ahc->isPrintingSteps())
            {
                (*out) << endl;
                if(this->ahc->debugMode()) // Additionnal line break for harmonious display
                    (*out) << endl;
            }
            
            list<InetAddress> targetsCp(ahcTargets);
            this->ahc->setIPsToProbe(ahcTargets);
            try
            {
                this->ahc->collect();
            }
            catch(StopException e)
            {
                throw;
            }
            
            // Pre-alias resolution
            Aggregate *tempAgg = new Aggregate(targetsCp);
            list<Aggregate*> *curAggs = cur->getAggregates();
            
            curAggs->push_back(tempAgg);
            ar->resolve(cur);
            curAggs->clear();
            
            // Filter initial results so only aliases based on UDP/Ally/IP-ID velocity stay
            list<Router*> *results = tempAgg->getInferredRouters();
            for(list<Router*>::iterator it = results->begin(); it != results->end(); ++it)
                this->filterPreAlias((*it));
            
            delete tempAgg;
            
            // Builds new Router objects
            IPLookUpTable *dict = env->getIPTable();
            list<Router*> preAliases;
            list<InetAddress> known; // Prevents duplicata
            for(list<InetAddress>::iterator it = targetsCp.begin(); it != targetsCp.end(); ++it)
            {
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
                
                list<InetAddress> *pres = ip->getPreAliases();
                if(pres->size() == 0)
                    continue;
                
                Router *newPreAlias = new Router();
                newPreAlias->addInterface((*it), RouterInterface::ALLY); // ALLY = placeholder
                known.push_back((*it));
                for(list<InetAddress>::iterator i = pres->begin(); i != pres->end(); ++i)
                {
                    newPreAlias->addInterface((*i), RouterInterface::ALLY); // ALLY = placeholder
                    known.push_back((*i));
                }
                preAliases.push_back(newPreAlias);
            }
            
            // Prints results
            size_t nbPreAliases = preAliases.size();
            if(nbPreAliases > 0)
            {
                (*out) << "Discovered " << nbPreAliases;
                if(nbPreAliases > 1)
                    (*out) << " pre-aliases:" << endl;
                else
                    (*out) << " pre-alias:" << endl;
                
                for(list<Router*>::iterator it = preAliases.begin(); it != preAliases.end(); ++it)
                {
                    Router *cur = (*it);
                    (*out) << "[" << cur->toString() << "]" << endl;
                    delete cur;
                }
                preAliases.clear();
            }
            else
            {
                (*out) << "No pre-alias discovered." << endl;
            }
            
            // Last thing to do: clean AR hints
            for(list<InetAddress>::iterator it = targetsCp.begin(); it != targetsCp.end(); ++it)
            {
                IPTableEntry *ip = dict->lookUp((*it));
                if(ip != NULL)
                    ip->resetARHints();
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

void Magpie::filterPreAlias(Router *preAlias)
{
    IPLookUpTable *dict = env->getIPTable();
    list<RouterInterface*> *IPs = preAlias->getInterfacesList();
    for(list<RouterInterface*>::iterator i = IPs->begin(); i != IPs->end(); i++)
    {
        RouterInterface *i1 = (*i);
        IPTableEntry *ip1 = dict->lookUp(i1->ip);
        if(ip1 == NULL)
            continue;
        
        for(list<RouterInterface*>::iterator j = i; j != IPs->end(); j++)
        {
            if(i == j)
                continue;
            
            RouterInterface *i2 = (*j);
            
            bool preAliasOK = false;
            if(i1->aliasMethod == RouterInterface::UDP_PORT_UNREACHABLE)
                preAliasOK = true;
            else if(i2->aliasMethod == RouterInterface::UDP_PORT_UNREACHABLE)
                preAliasOK = true;
            else if(i1->aliasMethod == RouterInterface::ALLY)
                preAliasOK = true;
            else if(i2->aliasMethod == RouterInterface::ALLY)
                preAliasOK = true;
            else if(i1->aliasMethod == RouterInterface::IPID_VELOCITY)
                preAliasOK = true;
            else if(i2->aliasMethod == RouterInterface::IPID_VELOCITY)
                preAliasOK = true;
            
            if(preAliasOK)
            {
                IPTableEntry *ip2 = dict->lookUp(i2->ip);
                
                if(ip2 == NULL)
                    continue;
                
                ip1->recordPreAlias(i2->ip);
                ip2->recordPreAlias(i1->ip);
            }
        }
    }
}
