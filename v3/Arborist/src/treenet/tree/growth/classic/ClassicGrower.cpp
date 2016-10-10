/*
 * ClassicGrower.cpp
 *
 *  Created on: Sept 9, 2016
 *      Author: grailet
 *
 * Implements the class defined in ClassicGrower.h (see this file to learn further about the goals 
 * of such class).
 */

#include <list>
using std::list;

#include "../../../../common/thread/Thread.h"
#include "ClassicGrower.h"
#include "ParisTracerouteTask.h"

ClassicGrower::ClassicGrower(TreeNETEnvironment *env) : Grower(env)
{
    /*
     * About maxDepth parameter: it is the size of the longest route to a subnet which should be 
     * inserted in the tree. It is used as the size of the depthMap array (i.e. one list of nodes 
     * per depth level), which should be maintained throughout the life of the tree to ease the 
     * insertion step (re-building the whole map at each insertion is costly).
     */

    unsigned short maxDepth = env->getSubnetSet()->getMaximumDistance();
    this->depthMap = new list<NetworkTreeNode*>[maxDepth];
    this->tree = NULL;
}

ClassicGrower::~ClassicGrower()
{
    delete[] depthMap;
}

void ClassicGrower::prepare()
{
    /*
     * PARIS TRACEROUTE
     *
     * The preparation of the classic growth method consists in computing the route to each 
     * inferred subnet, no matter how it was classified during previous algorithmic steps.
     *
     * The route computation itself is base on the same ideas as Paris Traceroute, and is 
     * parallelized to quickly obtain all routes.
     */
    
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnets = env->getSubnetSet();
    unsigned short nbThreads = env->getMaxThreads();
    unsigned short displayMode = env->getDisplayMode();
    
    list<SubnetSite*> *list = subnets->getSubnetSiteList();
    
    if(list->size() > 0)
    {
        (*out) << "Computing route to each subnet...\n" << endl;
        
        // Lists subnets for which we would like a route
        std::list<SubnetSite*> toSchedule;
        for(std::list<SubnetSite*>::iterator it = list->begin(); it != list->end(); ++it)
        {
            unsigned short status = (*it)->getRefinementStatus();
            if(status == SubnetSite::ACCURATE_SUBNET || 
               status == SubnetSite::SHADOW_SUBNET || 
               status == SubnetSite::ODD_SUBNET)
            {
                toSchedule.push_back((*it));
            }
        }
        
        // Size of the thread array
        unsigned short sizeParisArray = 0;
        if((unsigned long) toSchedule.size() > (unsigned long) nbThreads)
            sizeParisArray = nbThreads;
        else
            sizeParisArray = (unsigned short) toSchedule.size();
        
        // Creates thread(s)
        Thread **parisTh = new Thread*[sizeParisArray];
        for(unsigned short i = 0; i < sizeParisArray; i++)
            parisTh[i] = NULL;
        
        while(toSchedule.size() > 0)
        {
            unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
            range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
            range /= sizeParisArray;
            
            for(unsigned short i = 0; i < sizeParisArray && toSchedule.size() > 0; i++)
            {
                SubnetSite *curSubnet = toSchedule.front();
                toSchedule.pop_front();
                
                unsigned short lowBound = (i * range);
                unsigned short upBound = lowBound + range - 1;

                parisTh[i] = new Thread(new ParisTracerouteTask(env, 
                                                                curSubnet, 
                                                                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowBound, 
                                                                DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upBound, 
                                                                DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                                                DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ));
            }

            // Launches thread(s) then waits for completion
            for(unsigned short i = 0; i < sizeParisArray; i++)
            {
                if(parisTh[i] != NULL)
                {
                    parisTh[i]->start();
                    Thread::invokeSleep(env->getProbeThreadDelay());
                }
            }
            
            for(unsigned short i = 0; i < sizeParisArray; i++)
            {
                if(parisTh[i] != NULL)
                {
                    parisTh[i]->join();
                    delete parisTh[i];
                    parisTh[i] = NULL;
                }
            }
        }
        
        delete[] parisTh;
    }
    
    /*
     * If we are in laconic display mode, we add a line break before the next message to keep 
     * the display pretty to read, just like at the end of a Bypass round.
     */
    
    if(displayMode == TreeNETEnvironment::DISPLAY_MODE_LACONIC)
        (*out) << "\n";
    
    (*out) << "Finished computing routes.\n" << endl;
}

void ClassicGrower::grow()
{
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnets = env->getSubnetSet();

    subnets->removeArtifacts();
    subnets->sortByRoute();
    
    this->tree = new NetworkTree();

    // Inserting subnets with a complete route first
    SubnetSite *toInsert = subnets->getValidSubnet();
    while(toInsert != NULL)
    {
        this->insert(toInsert);
        
        toInsert = subnets->getValidSubnet();
    }
    
    // Always printed console messages
    (*out) << "Subnets with complete route inserted.\n";
    (*out) << "Now repairing incomplete routes to insert remaining subnets..." << endl;
    
    // Then, subnets with an incomplete route after a repairment
    toInsert = subnets->getValidSubnet(false);
    while(toInsert != NULL)
    {
        this->repairRoute(toInsert);
        this->insert(toInsert);
        
        toInsert = subnets->getValidSubnet(false);
    }
    
    // Creates the new Soil object and adds the produced tree in it
    this->result = new Soil();
    this->result->insertTree(this->tree);
    
    // Adds the subnet map entries in the Soil object
    this->result->insertMapEntries(newSubnetMapEntries);
    this->result->sortMapEntries();
    newSubnetMapEntries.clear();
}

void ClassicGrower::insert(SubnetSite *subnet)
{
    // Gets root of the tree
    NetworkTreeNode *rootNode = this->tree->getRoot();
    list<NetworkTreeNode*> *map = this->depthMap;

    // Gets route information of the new subnet
    RouteInterface *route = subnet->getRoute();
    unsigned short routeSize = subnet->getRouteSize();
    
    // Finds the deepest node which occurs in the route of current subnet
    NetworkTreeNode *insertionPoint = NULL;
    unsigned insertionPointDepth = 0;
    for(unsigned short d = routeSize; d > 0; d--)
    {
        if(route[d - 1].ip == RouteInterface::MISSING)
            continue;
    
        for(list<NetworkTreeNode*>::iterator i = map[d - 1].begin(); i != map[d - 1].end(); ++i)
        {
            if((*i)->hasLabel(route[d - 1].ip))
            {
                insertionPoint = (*i);
                insertionPointDepth = d;
                break;
            }
        }
        
        if(insertionPoint != NULL)
            break;
    }
    
    if(insertionPoint == NULL)
        insertionPoint = rootNode;
    
    // Creates the new branch
    NetworkTreeNode *subTree = createBranch(subnet, insertionPointDepth + 1);
    if(insertionPoint != rootNode)
        subTree->addPreviousLabel(route[insertionPointDepth - 1].ip);
    insertionPoint->addChild(subTree);

    // Puts the new internal nodes inside the depth map
    NetworkTreeNode *next = subTree;
    NetworkTreeNode *subnetNode = NULL; // For subnet map entry
    if(next->isInternal())
    {
        unsigned short curDepth = insertionPointDepth;
        do
        {
            map[curDepth].push_back(next);
            curDepth++;
            
            list<NetworkTreeNode*> *children = next->getChildren();
            if(children != NULL && children->size() == 1)
            {
                if(children->front()->isInternal())
                {
                    next = children->front();
                }
                else
                {
                    subnetNode = children->front();
                    next = NULL;
                }
            }
            else
                next = NULL;
        }
        while(next != NULL);
    }
    
    /*
     * If nodes "above" the insertion point do not share the exact same route as the one we 
     * obtained for this subnet, we add the interfaces at a same TTL (i.e. depth) that differ 
     * from what we already have.
     */
    
    NetworkTreeNode *cur = insertionPoint->getParent();
    NetworkTreeNode *child = insertionPoint;
    for(unsigned short d = insertionPointDepth; d > 1; d--)
    {
        child->addPreviousLabel(route[d - 2].ip);
    
        if(!cur->hasLabel(route[d - 2].ip))
        {
            cur->addLabel(route[d - 2].ip);
            
            /*
             * Look in depth map for a node at same depth sharing the new label. Indeed, if such
             * node exists, we should merge it with the current node, otherwise we will have 2 
             * nodes sharing a same label in the tree, which should not occur if we want the tree 
             * to be fidel to the topology.
             */
            
            NetworkTreeNode *toMerge = NULL;
            for(list<NetworkTreeNode*>::iterator i = map[d - 2].begin(); i != map[d - 2].end(); ++i)
            {
                if((*i) != cur && (*i)->hasLabel(route[d - 2].ip))
                {
                    toMerge = (*i);
                    break;
                }
            }
            
            if(toMerge != NULL)
            {
                cur->merge(toMerge);
                
                // Add labels in toMerge absent from cur
                list<InetAddress> *labels1 = cur->getLabels();
                list<InetAddress> *labels2 = toMerge->getLabels();
                labels1->merge((*labels2));
                labels1->sort(InetAddress::smaller);
                InetAddress prev(0);
                bool first = true; // Avoids incoherent behavior with labels 0.0.0.0
                for(list<InetAddress>::iterator i = labels1->begin(); i != labels1->end(); ++i)
                {
                    if(!first)
                    {
                        if((*i) == prev)
                        {
                            labels1->erase(i--);
                        }
                    }
                    prev = (*i);
                    first = false;
                }
                
                this->prune(toMerge, NULL, d - 2);
                
                // Sorts parent's children to keep the good order (it can change after merging)
                cur->getParent()->sortChildren();
            }
        }
    
        child = cur;
        cur = cur->getParent();
    }
    
    // Last step: creates an entry for the subnet map (for look-up)
    newSubnetMapEntries.push_back(new SubnetMapEntry(subnet, tree, subnetNode));
}

void ClassicGrower::prune(NetworkTreeNode *cur, NetworkTreeNode *prev, unsigned short depth)
{
    list<NetworkTreeNode*> *map = this->depthMap;

    if(cur->isLeaf())
    {
        if(prev != NULL)
            delete prev;
        return;
    }
    else if(cur->isRoot())
    {
        if(prev != NULL)
        {
            list<NetworkTreeNode*> *children = cur->getChildren();
            for(list<NetworkTreeNode*>::iterator i = map[0].begin(); i != map[0].end(); ++i)
            {
                if((*i) == prev)
                {
                    map[0].erase(i);
                    break;
                }
            }
        
            // Erases prev from the children list (of this node) and stops
            for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
            {
                if((*i) == prev)
                {
                    delete (*i);
                    children->erase(i);
                    return;
                }
            }
        
            delete prev;
        }
        return;
    }
    
    list<NetworkTreeNode*> *children = cur->getChildren();

    // Current node has multiple children; keep it but remove prev
    if(children->size() > 1)
    {
        // Erases prev from the depth map
        unsigned dPrev = depth + 1;
        for(list<NetworkTreeNode*>::iterator i = map[dPrev].begin(); i != map[dPrev].end(); ++i)
        {
            if((*i) == prev)
            {
                map[dPrev].erase(i);
                break;
            }
        }
    
        // Erases prev from the children list (of this node) and stops
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            if((*i) == prev)
            {
                delete (*i);
                children->erase(i);
                return;
            }
        }
    }
    // Current node has a single child: prev. Deletes it and moves up in the tree.
    else if(children->size() == 1)
    {
        // Erases prev from the depth map
        unsigned dPrev = depth + 1;
        for(list<NetworkTreeNode*>::iterator i = map[dPrev].begin(); i != map[dPrev].end(); ++i)
        {
            if((*i) == prev)
            {
                map[dPrev].erase(i);
                break;
            }
        }
    
        delete prev;
        cur->getChildren()->clear();
        this->prune(cur->getParent(), cur, depth - 1);
    }
    // Current node has no longer children. Moves up in the tree.
    else if(children->size() == 0)
    {
        this->prune(cur->getParent(), cur, depth - 1);
    }
}

void ClassicGrower::repairRoute(SubnetSite *ss)
{
    list<NetworkTreeNode*> *map = this->depthMap;
    RouteInterface *route = ss->getRoute();
    unsigned short routeSize = ss->getRouteSize();

    // Finds deepest match in the tree
    NetworkTreeNode *insertionPoint = NULL;
    unsigned insertionPointDepth = 0;
    for(unsigned short d = routeSize; d > 0; d--)
    {
        if(route[d - 1].state == RouteInterface::MISSING)
            continue;
    
        for(list<NetworkTreeNode*>::iterator i = map[d - 1].begin(); i != map[d - 1].end(); ++i)
        {
            if((*i)->hasLabel(route[d - 1].ip))
            {
                insertionPoint = (*i);
                insertionPointDepth = d;
                break;
            }
        }
        
        if(insertionPoint != NULL)
            break;
    }
    
    // If the insertion point could not be found, nothing can be done.
    if(insertionPoint == NULL)
    {
        return;
    }
    
    // Lists all subnets which belongs to the branch where insertion point is.
    list<SubnetSite*> subnetList;
    listSubnetsRecursive(&subnetList, insertionPoint);
    
    // Finds the route which is the most similar to the incomplete route
    RouteInterface *similarRoute = NULL;
    unsigned short maxSimilarities = 0;
    for(list<SubnetSite*>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
    {
        SubnetSite *cur = (*i);
        
        if(!cur->hasCompleteRoute())
            continue;
        
        RouteInterface *curRoute = cur->getRoute();
        
        unsigned short similarities = 0;
        for(unsigned short j = 0; j < insertionPointDepth; ++j)
        {
            if(curRoute[j].ip == route[j].ip)
                similarities++;
        }
        
        if(similarities > maxSimilarities)
        {
            similarRoute = curRoute;
            maxSimilarities = similarities;
        }
    }
    
    // If there was no subnet with a complete similar route, nothing can be done.
    if(similarRoute == NULL)
    {
        return;
    }
    
    // Completes the incomplete route
    for(unsigned short i = 0; i < insertionPointDepth; ++i)
    {
        if(route[i].state == RouteInterface::MISSING)
        {
            route[i].repair(similarRoute[i].ip);
        }
    }
}

NetworkTreeNode *ClassicGrower::createBranch(SubnetSite *subnet, unsigned short depth)
{
    RouteInterface *route = subnet->getRoute();
    unsigned short routeSize = subnet->getRouteSize();
    
    // If current depth minus 1 equals the route size, then we just have to create a leaf
    if((depth - 1) == routeSize)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(subnet);
        return newNode;
    }

    // Creates the branch for this new subnet, as a cascade of new nodes with missing interfaces
    NetworkTreeNode *newRoot = new NetworkTreeNode(route[depth - 1].ip);
    NetworkTreeNode *curNode = newRoot;
    for(int i = depth + 1; i <= routeSize; i++)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(route[i - 1].ip);
        newNode->addPreviousLabel(route[i - 2].ip);
        curNode->addChild(newNode);
        curNode = newNode;
    }
    curNode->addChild(new NetworkTreeNode(subnet));
    
    return newRoot;
}

void ClassicGrower::listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur)
{
    // Subnets are stored in leaves
    if(cur->isLeaf())
    {
        subnetsList->push_back(cur->getAssociatedSubnet());
        return;
    }
    
    // Goes deeper in the tree
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        listSubnetsRecursive(subnetsList, (*i));
    }
}
