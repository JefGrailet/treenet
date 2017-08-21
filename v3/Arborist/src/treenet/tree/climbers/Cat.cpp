/*
 * Cat.cpp
 *
 *  Created on: Nov 3, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Cat.h (see this file to learn further about the goals of such 
 * class).
 */

#include <list>
using std::list;

#include "Cat.h"

Cat::Cat(TreeNETEnvironment *env) : Climber(env)
{
}

Cat::~Cat()
{
}

void Cat::climb(Soil *fromSoil)
{
    this->soilRef = fromSoil;
    
    ostream *out = env->getOutputStream();
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    
    if(roots->size() > 1)
    {
        unsigned short treeIndex = 1;
        
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            (*out) << "[Tree n°" << treeIndex << "]\n" << endl;
            this->climbRecursive((*i)->getRoot(), 0);
            
            treeIndex++;
        }
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot(), 0);
    }
    
    this->soilRef = NULL;
}

void Cat::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
{
    ostream *out = env->getOutputStream();
    
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    size_t nbChildren = children->size();
    
    // Ingress/egress interfaces count (for amount of interfaces inference)
    unsigned int ingressInterfaces = 1;
    NetworkTreeNode *parent = cur->getParent();
    if(parent != NULL && parent->isInternal())
    {
        ingressInterfaces = parent->getLabels()->size();
    }
    unsigned int egressInterfaces = 0;
    
    // Puts direct neighbor subnets in a list and puts the internal nodes in another one
    list<NetworkTreeNode*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        // Just in case (PlanetLab...)
        if((*i) == NULL)
            continue;
    
        if((*i)->isLeaf())
        {
            childrenL.push_back((*i));
            egressInterfaces++;
        }
        else if((*i)->isInternal())
        {
            childrenI.push_back((*i));
            egressInterfaces += (*i)->getLabels()->size();
        }
    }
    
    size_t nbNeighborSubnets = childrenL.size();
    unsigned short countChildrenI = (unsigned short) childrenI.size();

    if(nbNeighborSubnets > 0)
    {
        if(cur->isRoot())
        {
            (*out) << "Neighboring subnets of this computer:" << endl;
        }
        else
        {
            if (cur->isHedera())
            {
                (*out) << "Neighboring subnets of Hedera {";
                bool guardian = false;
                for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                {
                    if (guardian)
                        (*out) << ", ";
                    else
                        guardian = true;
                
                    (*out) << (*i);
                }
                (*out) << "}:" << endl;
            }
            else
            {
                (*out) << "Neighboring subnets of Neighborhood " << labels->front() << ":" << endl;
            }
        }
        
        for(list<NetworkTreeNode*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
        {
            // Just in case (PlanetLab...)
            if((*i) == NULL)
                continue;
            
            SubnetSite *ss = (*i)->getAssociatedSubnet();
            
            // Just in case (PlanetLab...)
            if(ss == NULL)
                continue;
                
            string subnetStr = ss->getInferredNetworkAddressString();
            unsigned short status = ss->getStatus();
            bool credible = ss->isCredible();
            
            (*out) << subnetStr << " (";
            if(status == SubnetSite::ACCURATE_SUBNET)
                (*out) << "ACCURATE";
            else if(status == SubnetSite::ODD_SUBNET)
                (*out) << "ODD";
            else
                (*out) << "SHADOW";
            if(credible)
                (*out) << ", credible";
            (*out) << ")";
            
            /*
             * This part of code verifies that each child subnet is crossed to reach one or 
             * several children which are internal nodes. This also checks if the node has partial 
             * or complete linkage with its children.
             */
            
            if(childrenI.size() > 0)
            {
                list<InetAddress> onTheWay;
                for(list<NetworkTreeNode*>::iterator j = childrenI.begin(); j != childrenI.end(); ++j)
                {
                    // Just in case
                    if((*j) == NULL)
                        continue;
                
                    list<InetAddress> *labels = (*j)->getLabels();
                    
                    // Idem
                    if(labels == NULL)
                        continue;
                    
                    for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                    {
                        if(ss->contains((*k)))
                        {
                            onTheWay.push_back((*k));
                            
                            // Due to Hedera's, countChildrenI can eventually rollover
                            if(countChildrenI > 0)
                                countChildrenI--;
                            
                            egressInterfaces--; // Decremented for each "on the way" child internal
                        }
                    }
                }
                
                if(onTheWay.size() > 0)
                {
                    (*out) << ", on the way to ";
                    bool first = true;
                    for(list<InetAddress>::iterator j = onTheWay.begin(); j != onTheWay.end(); ++j)
                    {
                        if(!first)
                            (*out) << ", ";
                        else
                            first = false;
                        
                        (*out) << (*j);
                    }
                }
            }
            
            (*out) << endl;
        }
        
        (*out) << "Inferred a total " << egressInterfaces + ingressInterfaces << " interfaces.\n";
        if(countChildrenI == 0)
        {
            (*out) << "This internal node has complete linkage with its children." << endl;
        }
        else if(countChildrenI == 1)
        {
            (*out) << "This internal node has partial linkage with its children." << endl;
        }
        
        /*
         * The code now checks that each label of the internal node belongs to a subnet which is 
         * present within the tree (due to routing, it may appear in other branches than expected).
         */
        
        if(!cur->isRoot())
        {
            (*out) << "\nLabel analysis:" << endl;
            unsigned short nbLabels = labels->size();
            unsigned short appearingLabels = 0;
            for(list<InetAddress>::iterator l = labels->begin(); l != labels->end(); ++l)
            {
                InetAddress curLabel = (*l);
                SubnetMapEntry *container = this->soilRef->getSubnetContaining(curLabel);
                if(container != NULL)
                {
                    (*out) << "Label " << curLabel << " belongs to registered subnet ";
                    (*out) << container->subnet->getInferredNetworkAddressString() << endl;
                    appearingLabels++;
                }
                else
                {
                    (*out) << "Label " << curLabel << " does not belong to any registered subnet ";
                    (*out) << endl;
                }
            }
            if(appearingLabels == nbLabels)
            {
                (*out) << "All labels belong to subnets appearing in the tree." << endl;
            }
        }
        
        (*out) << endl;
        
        // Fingerprints
        list<Aggregate*> *aggs = cur->getAggregates();
        if(cur->isHedera() && aggs->size() > 1)
        {
            (*out) << "Fingerprints (sorted, grouped by last hop towards interface):\n";
            for(list<Aggregate*>::iterator it = aggs->begin(); it != aggs->end(); ++it)
            {
                list<Fingerprint> *prints = (*it)->getFingerprints();
                (*out) << "\nLast hop = " << (*it)->getFirstLastHop() << ":\n";
                for(list<Fingerprint>::iterator it2 = prints->begin(); it2 != prints->end(); ++it2)
                    (*out) << (InetAddress) (*((*it2).ipEntry)) << " - " << (*it2) << "\n";
            }
            (*out) << endl;
        }
        else
        {
            list<Fingerprint> *prints = aggs->front()->getFingerprints();
            if(prints->size() > 1)
            {
                (*out) << "Fingerprints (sorted):\n";
                for(list<Fingerprint>::iterator it = prints->begin(); it != prints->end(); ++it)
                    (*out) << (InetAddress) (*((*it).ipEntry)) << " - " << (*it) << "\n";
                (*out) << endl;
            }
        }
        
        // Routers
        list<Router*> routers = cur->getInferredRouters();
        unsigned short nbRouters = (unsigned short) routers.size();
        if(nbRouters > 0)
        {
            if(nbRouters == 1)
                (*out) << "Inferred router: " << endl;
            else
                (*out) << "Inferred routers: " << endl;
            
            for(list<Router*>::iterator i = routers.begin(); i != routers.end(); ++i)
            {
                (*out) << "[" << (*i)->toStringVerbose() << "]" << endl;
            }
        }
        else
        {
            (*out) << "Data is lacking for credible router inference." << endl;
        }
        
        (*out) << endl;
        
        (*out) << "------------------------------------------" << endl << endl;
    }
    // More than 2 children needed to legitimate display
    else if(nbChildren > 1)
    {
        if(cur->isRoot())
        {
            (*out) << "Neighborhood of this computer:" << endl;
        }
        else
        {
            if(cur->isHedera())
            {
                (*out) << "Hedera of internal node {";
                bool guardian = false;
                for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                {
                    if (guardian)
                        (*out) << ", ";
                    else
                        guardian = true;
                
                    (*out) << (*i);
                }
                (*out) << "}:" << endl;
            }
            else
            {
                (*out) << "Neighborhood of internal node " << labels->front() << ":" << endl;
            }
        }
        
        unsigned int total = egressInterfaces + ingressInterfaces;
        unsigned short linkage = cur->getLinkage();
        
        (*out) << "No neighboring subnets, but inferred a total of " << total << " interfaces.\n";
        if(linkage == 0)
        {
            (*out) << "This internal node has complete linkage with its children." << endl;
        }
        else if(linkage == 1)
        {
            (*out) << "This internal node has partial linkage with its children." << endl;
        }
        
        /*
         * The code now checks that each label of the internal node belongs to a subnet which is 
         * present within the tree (due to routing, it may appear in other branches than expected).
         */
        
        if(cur->isRoot())
        {
            (*out) << endl;
        }
        else
        {
            (*out) << endl << "Label analysis:" << endl;
            unsigned short nbLabels = labels->size();
            unsigned short appearingLabels = 0;
            for(list<InetAddress>::iterator l = labels->begin(); l != labels->end(); ++l)
            {
                InetAddress curLabel = (*l);
                SubnetMapEntry *container = this->soilRef->getSubnetContaining(curLabel);
                if(container != NULL)
                {
                    (*out) << "Label " << curLabel << " belongs to registered subnet ";
                    (*out) << container->subnet->getInferredNetworkAddressString() << endl;
                    appearingLabels++;
                }
                else
                {
                    (*out) << "Label " << curLabel << " does not belong to any registered subnet ";
                    (*out) << endl;
                }
            }
            if(appearingLabels == nbLabels)
            {
                (*out) << "All labels belong to subnets appearing in the tree." << endl;
            }
            (*out) << endl;
        }
        
        (*out) << "------------------------------------------" << endl << endl;
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i) != NULL && (*i)->isInternal())
            this->climbRecursive((*i), depth + 1);
    }
}
