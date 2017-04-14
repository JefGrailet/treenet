/*
 * GrafterGrower.cpp
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in GrafterGrower.h (see this file to learn further about the goals 
 * of such class).
 */

#include <list>
using std::list;

#include "GrafterGrower.h"

GrafterGrower::GrafterGrower(TreeNETEnvironment *env) : Grower(env)
{
    /*
     * About maxDepth parameter: it is the size of the longest route to a subnet which should be 
     * inserted in the tree. It is used as the size of the depthMap array (i.e. one list of nodes 
     * per depth level), which should be maintained throughout the life of the tree to ease the 
     * insertion step (re-building the whole map at each insertion is costly).
     */

    maxDepth = env->getSubnetSet()->getMaximumDistance();
    this->depthMap = new list<NetworkTreeNode*>[maxDepth];
    this->tree = NULL;
}

GrafterGrower::GrafterGrower(TreeNETEnvironment *env, unsigned short maxDepth) : Grower(env)
{
    this->maxDepth = maxDepth;
    this->depthMap = new list<NetworkTreeNode*>[maxDepth];
    this->tree = NULL;
}

GrafterGrower::~GrafterGrower()
{
    delete[] depthMap;
}


void GrafterGrower::prepare()
{
    // Should not be used
}

void GrafterGrower::grow()
{
    SubnetSiteSet *subnets = env->getSubnetSet();

    subnets->sortByRoute();
    
    this->tree = new NetworkTree();

    // Inserting subnets with a complete route first
    SubnetSite *toInsert = subnets->getValidSubnet();
    while(toInsert != NULL)
    {
        this->insert(toInsert);
        
        toInsert = subnets->getValidSubnet();
    }
    
    // Then, subnets with an incomplete route
    toInsert = subnets->getValidSubnet(false);
    while(toInsert != NULL)
    {
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

void GrafterGrower::insert(SubnetSite *subnet)
{
    // Gets root of the tree
    NetworkTreeNode *rootNode = this->tree->getRoot();
    list<NetworkTreeNode*> *map = this->depthMap;

    // Gets route information of the new subnet
    unsigned short routeSize;
    RouteInterface *route = subnet->getFinalRoute(&routeSize);
    
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

void GrafterGrower::prune(NetworkTreeNode *cur, NetworkTreeNode *prev, unsigned short depth)
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

NetworkTreeNode *GrafterGrower::createBranch(SubnetSite *subnet, unsigned short depth)
{
    unsigned short routeSize;
    RouteInterface *route = subnet->getFinalRoute(&routeSize);
    
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

bool GrafterGrower::matchesTrunk(SubnetSite *subnet)
{
    // Gets route details
    unsigned short routeSize;
    RouteInterface *route = subnet->getFinalRoute(&routeSize);

    // Goes through the main trunk
    NetworkTreeNode *trunkEnd = tree->getRoot();
    unsigned short routeIndex = 0;
    while(trunkEnd != NULL && !trunkEnd->isLeaf() && trunkEnd->getChildren()->size() == 1)
    {
        trunkEnd = trunkEnd->getChildren()->front();
        
        if(trunkEnd->hasLabel(route[routeIndex].ip))
            return true;
        
        routeIndex++;
        if(routeIndex == routeSize)
            break;
    }
    
    return false;
}

bool GrafterGrower::isGraftable(SubnetSite *ss, 
                                unsigned short *sOld, 
                                InetAddress **oldPrefix, 
                                unsigned short *sNew, 
                                InetAddress **newPrefix)
{
    // Gets route information of the new subnet
    unsigned short routeSize;
    RouteInterface *route = ss->getFinalRoute(&routeSize);
    
    // Finds the earliest node which has a label occurring in route (first interfaces first)
    NetworkTreeNode *matchingPoint = NULL;
    unsigned matchingPointDepth = 0; // Depth in the tree
    unsigned matchingIndex = 0; // Index in route
    for(unsigned short i = 0; i < routeSize; i++)
    {
        if(route[i].ip == InetAddress(0))
            continue;
        
        for(unsigned short j = 0; j < maxDepth; j++)
        {
            list<NetworkTreeNode*> curLs = this->depthMap[j];
            for(list<NetworkTreeNode*>::iterator it = curLs.begin(); it != curLs.end(); ++it)
            {
                if((*it)->hasLabel(route[i].ip))
                {
                    matchingPoint = (*it);
                    matchingPointDepth = j;
                    matchingIndex = i;
                    break;
                }
            }
            
            if(matchingPoint != NULL)
                break;
        }
        
        if(matchingPoint != NULL)
            break;
    }

    if (matchingPoint == NULL)
        return false;
    
    // Allocates memory to store old and new route prefix
    (*oldPrefix) = new InetAddress[matchingIndex];
    (*newPrefix) = new InetAddress[matchingPointDepth];
    (*sOld) = matchingIndex;
    (*sNew) = matchingPointDepth;
    
    // Writes the routes
    InetAddress *oldRoute = (*oldPrefix);
    for(unsigned short i = 0; i < (*sOld); i++)
        oldRoute[i] = route[i].ip;
    
    InetAddress *newRoute = (*newPrefix);
    NetworkTreeNode *cur = matchingPoint->getParent();
    for(unsigned short i = 0; i < (*sNew); i++)
    {
        unsigned short index = (*sNew) - 1 - i;
        list<InetAddress> *labels = cur->getLabels();
        if(labels->size() == 1)
        {
            newRoute[index] = labels->front();
        }
        // If multiple labels, take the first that is different than 0.0.0.0
        else
        {
            bool assigned = false;
            for(list<InetAddress>::iterator it = labels->begin(); it != labels->end(); ++it)
            {
                if((*it) != InetAddress(0))
                {
                    newRoute[index] = (*it);
                    assigned = true;
                    break;
                }
            }
            
            // Should theoretically not occur, but just in case...
            if(!assigned)
                newRoute[index] = labels->front();
        }
        cur = cur->getParent();
    }

    return true;
}

bool GrafterGrower::graft(SubnetSite *ss)
{
    ostream *out = env->getOutputStream();

    bool successfullyGrafted = false;
    bool fitting = this->matchesTrunk(ss);
    if(!fitting)
    {
        unsigned short oldPrefixSize = 0, newPrefixSize = 0;
        InetAddress *oldPrefix = NULL, *newPrefix = NULL;
        
        bool graftable = this->isGraftable(ss, 
                                           &oldPrefixSize, 
                                           &oldPrefix, 
                                           &newPrefixSize, 
                                           &newPrefix);
        
        if(graftable)
        {
            unsigned short res = 1;
            ss->adaptRoute(oldPrefixSize, newPrefixSize, newPrefix);
            res += env->getSubnetSet()->adaptRoutes(oldPrefixSize, 
                                                    oldPrefix, 
                                                    newPrefixSize, 
                                                    newPrefix);
            
            // Prints out the transplantation that occurred
            (*out) << "Grafting:\nOriginal route prefix: ";
            for(unsigned short i = 0; i < oldPrefixSize; i++)
            {
                if(i > 0)
                    (*out) << ", ";
                (*out) << oldPrefix[i];
            }
            (*out) << "\nPredicted route prefix: ";
            for(unsigned short i = 0; i < newPrefixSize; i++)
            {
                if(i > 0)
                    (*out) << ", ";
                (*out) << newPrefix[i];
            }
            (*out) << "\n";
            
            if(res > 1)
                (*out) << res << " grafted subnets.\n";
            else
                (*out) << "One grafted subnet.\n";
            (*out) << endl;
            
            this->insert(ss);
            successfullyGrafted = true;
        }
        
        if(oldPrefix != NULL)
            delete[] oldPrefix;
        
        if(newPrefix != NULL)
            delete[] newPrefix;
    }
    else
    {
        this->insert(ss);
        successfullyGrafted = true;
    }
    
    return successfullyGrafted;
}

void GrafterGrower::flushMapEntries()
{
    if(this->result != NULL)
    {
        this->result->insertMapEntries(newSubnetMapEntries);
        this->result->sortMapEntries();
        newSubnetMapEntries.clear();
    }
}
