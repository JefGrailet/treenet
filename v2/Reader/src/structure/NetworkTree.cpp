/*
 * NetworkTree.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
 *
 * Implements the class defined in NetworkTree.h (see this file to learn further about the goals 
 * of such class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include <iomanip>
using std::left;
using std::right;

using namespace std;

#include "NetworkTree.h"
#include "Router.h"
#include "../common/thread/Thread.h" // For invokeSleep()
#include "../bipartite/BipartiteSwitch.h"
#include "../bipartite/BipartiteRouter.h"
#include "../bipartite/BipartiteSubnet.h"
#include "../aliasresolution/Fingerprint.h" // See internalsRecursive()

NetworkTree::NetworkTree(unsigned short maxDepth)
{
    this->root = new NetworkTreeNode();
    this->depthMap = new list<NetworkTreeNode*>[maxDepth];
    this->maxDepth = maxDepth;
    this->subnetMap = new list<SubnetSite*>[SIZE_SUBNET_MAP];
}

NetworkTree::~NetworkTree()
{
    delete root;
    delete[] depthMap;
    delete[] subnetMap;
}

void NetworkTree::insert(SubnetSite *subnet)
{
    // Gets route information of the new subnet
    InetAddress *route = subnet->getRoute();
    unsigned short routeSize = subnet->getRouteSize();
    
    // Finds the deepest node which occurs in the route of current subnet
    NetworkTreeNode *insertionPoint = NULL;
    unsigned insertionPointDepth = 0;
    for(unsigned short d = routeSize; d > 0; d--)
    {
        if(route[d - 1] == InetAddress("0.0.0.0"))
            continue;
    
        for(list<NetworkTreeNode*>::iterator i = this->depthMap[d - 1].begin(); i != this->depthMap[d - 1].end(); ++i)
        {
            if((*i)->hasLabel(route[d - 1]))
            {
                insertionPoint = (*i);
                insertionPointDepth = d;
                break;
            }
        }
        
        if(insertionPoint != NULL)
            break;
    }

    if (insertionPoint == NULL)
        insertionPoint = root;
    
    // Creates the new branch
    NetworkTreeNode *subTree = insertSubnet(subnet, insertionPointDepth + 1);
    if(insertionPoint != root)
        subTree->addPreviousLabel(route[insertionPointDepth - 1]);
    insertionPoint->addChild(subTree);

    // Puts the new (T_NEIGHBORHOOD) nodes inside the depth map
    NetworkTreeNode *next = subTree;
    if(next->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
    {
        unsigned short curDepth = insertionPointDepth;
        do
        {
            this->depthMap[curDepth].push_back(next);
            curDepth++;
            
            list<NetworkTreeNode*> *children = next->getChildren();
            if(children != NULL && children->size() == 1)
            {
                if(children->front()->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
                    next = children->front();
                else
                    next = NULL;
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
        child->addPreviousLabel(route[d - 2]);
    
        if(!cur->hasLabel(route[d - 2]))
        {
            cur->addLabel(route[d - 2]);
            
            /*
             * Look in depth map for a node at same depth sharing the new label. Indeed, if such
             * node exists, we should merge it with the current node, otherwise we will have 2 
             * nodes sharing a same label in the tree, which should not occur if we want the tree 
             * to be faithful to the topology.
             */
            
            NetworkTreeNode *toMerge = NULL;
            for(list<NetworkTreeNode*>::iterator i = this->depthMap[d - 2].begin(); i != this->depthMap[d - 2].end(); ++i)
            {
                if((*i) != cur && (*i)->hasLabel(route[d - 2]))
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
                
                prune(this->depthMap, toMerge, NULL, d - 2);
                
                // Sorts parent's children to keep the good order (it can change after merging)
                cur->getParent()->sortChildren();
            }
        }
    
        child = cur;
        cur = cur->getParent();
    }
    
    // Last step: insertion in the subnet map (for look-up)
    InetAddress baseIP = subnet->getInferredSubnetBaseIP();
    unsigned long index = (baseIP.getULongAddress() >> 12);
    this->subnetMap[index].push_back(subnet);
}

void NetworkTree::visit(ostream *out)
{
    visitRecursive(out, root, 0);
}

unsigned int *NetworkTree::getStatistics()
{
    unsigned int *stat = new unsigned int[5];
    for(unsigned int i = 0; i < 5; i++)
        stat[i] = 0;
    
    statisticsRecursive(stat, this, root);
    
    return stat;
}

void NetworkTree::repairRoute(SubnetSite *ss)
{
    // Finds deepest match in the tree
    InetAddress *route = ss->getRoute();
    unsigned short routeSize = ss->getRouteSize();
    NetworkTreeNode *insertionPoint = NULL;
    unsigned insertionPointDepth = 0;
    for(unsigned short d = routeSize; d > 0; d--)
    {
        if(route[d - 1] == InetAddress("0.0.0.0"))
            continue;
    
        for(list<NetworkTreeNode*>::iterator i = this->depthMap[d - 1].begin(); i != this->depthMap[d - 1].end(); ++i)
        {
            if((*i)->hasLabel(route[d - 1]))
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
    InetAddress *similarRoute = NULL;
    unsigned short maxSimilarities = 0;
    for(list<SubnetSite*>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
    {
        SubnetSite *cur = (*i);
        
        if(!cur->hasCompleteRoute())
            continue;
        
        InetAddress *curRoute = cur->getRoute();
        
        unsigned short similarities = 0;
        for(unsigned short j = 0; j < insertionPointDepth; ++j)
        {
            if(curRoute[j] == route[j])
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
    unsigned short *editMask = new unsigned short[routeSize];
    for(unsigned short i = 0; i < routeSize; ++i)
        editMask[i] = false;
    
    for(unsigned short i = 0; i < insertionPointDepth; ++i)
    {
        if(route[i] == InetAddress("0.0.0.0"))
        {
            route[i] = similarRoute[i];
            editMask[i] = SubnetSite::REPAIRED_INTERFACE;
        }
    }
    
    ss->setRouteEditMask(editMask);
}

void NetworkTree::collectAliasResolutionHints(ostream *out, AliasHintCollector *ahc)
{
    // Small delay before starting with the first internal (typically half a second)
    Thread::invokeSleep(ahc->getEnvironment()->getProbeThreadDelay() * 2);

    collectHintsRecursive(out, ahc, root, 0);
}

SubnetSite *NetworkTree::getSubnetContaining(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<SubnetSite*> subnetList = this->subnetMap[index];
    
    for(list<SubnetSite*>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        if(ss->contains(needle))
            return ss;
    }
    return NULL;
}

void NetworkTree::inferRouters(AliasResolver *ar)
{
    inferRoutersRecursive(this, root, ar, 0);
}

void NetworkTree::internals(ostream *out)
{
    internalsRecursive(out, this, root);
}

unsigned int NetworkTree::largestFingerprintList()
{
    unsigned int largestSize = 0;
    largestListRecursive(root, &largestSize);
    return largestSize;
}

void NetworkTree::outputSubnets(string filename)
{
    string output = "";
    list<SubnetSite*> siteList;
    listSubnetsRecursive(&siteList, root);
    siteList.sort(SubnetSite::compare);
    
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        string cur = ss->toString();
        
        if(!cur.empty())
            output += cur + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void NetworkTree::outputAliases(string filename)
{
    string output = "";
    outputAliasesRecursive(root, &output);
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

unsigned short NetworkTree::getTrunkSize()
{
    unsigned short size = 0;
    NetworkTreeNode *cur = root;
    while(cur != NULL && !cur->isLeaf() && cur->getChildren()->size() == 1)
    {
        cur = cur->getChildren()->front();
        size++;
    }
    return size;
}

bool NetworkTree::hasIncompleteTrunk()
{
    NetworkTreeNode *cur = root;
    while(cur != NULL && !cur->isLeaf() && cur->getChildren()->size() == 1)
    {
        cur = cur->getChildren()->front();
        if(cur->getLabels()->size() == 1 && cur->getLabels()->front() == InetAddress("0.0.0.0"))
            return true;
    }
    return false;
}

list<InetAddress> NetworkTree::listInterfacesAfterTrunk()
{
    list<InetAddress> result;

    // Gets to end of trunk
    NetworkTreeNode *trunkEnd = root;
    while(trunkEnd != NULL && !trunkEnd->isLeaf() && trunkEnd->getChildren()->size() == 1)
    {
        trunkEnd = trunkEnd->getChildren()->front();
    }
    
    // Returns empty list in case of problem or single-leaf tree
    if(trunkEnd == NULL || trunkEnd->isLeaf())
        return result;
    
    listInterfacesRecursive(&result, trunkEnd);
    result.sort(InetAddress::smaller);
    
    // Removes potential duplicates (rare but possible)
    InetAddress previous("0.0.0.0");
    for(list<InetAddress>::iterator i = result.begin(); i != result.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == previous)
        {
            result.erase(i--);
        }
        else
        {
            previous = cur;
        }
    }
    
    return result;
}

void NetworkTree::nullifyLeaves(SubnetSiteSet *sink)
{
    nullifyLeavesRecursive(sink, root);
}

bool NetworkTree::fittingRoute(SubnetSite *ss)
{
    // Gets route details
    unsigned short routeSize = ss->getRouteSize();
    InetAddress *route = ss->getRoute();

    // Goes through the main trunk
    NetworkTreeNode *trunkEnd = root;
    unsigned short routeIndex = 0;
    while(trunkEnd != NULL && !trunkEnd->isLeaf() && trunkEnd->getChildren()->size() == 1)
    {
        trunkEnd = trunkEnd->getChildren()->front();
        
        if(trunkEnd->hasLabel(route[routeIndex]))
            return true;
        
        routeIndex++;
        if(routeIndex == routeSize)
            break;
    }
    
    return false;
}

bool NetworkTree::findTransplantation(SubnetSite *ss, 
                                      unsigned short *sOld, 
                                      InetAddress **oldPrefix, 
                                      unsigned short *sNew, 
                                      InetAddress **newPrefix)
{
    // Gets route information of the new subnet
    InetAddress *route = ss->getRoute();
    unsigned short routeSize = ss->getRouteSize();
    
    // Finds the earliest node which has a label occurring in route (first interfaces first)
    NetworkTreeNode *matchingPoint = NULL;
    unsigned matchingPointDepth = 0; // Depth in the tree
    unsigned matchingIndex = 0; // Index in route
    for(unsigned short i = 0; i < routeSize; i++)
    {
        if(route[i] == InetAddress("0.0.0.0"))
            continue;
        
        for(unsigned short j = 0; j < maxDepth; j++)
        {
            list<NetworkTreeNode*> curLs = this->depthMap[j];
            for(list<NetworkTreeNode*>::iterator it = curLs.begin(); it != curLs.end(); ++it)
            {
                if((*it)->hasLabel(route[i]))
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
        oldRoute[i] = route[i];
    
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
                if((*it) != InetAddress("0.0.0.0"))
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

BipartiteGraph *NetworkTree::toBipartite()
{
    BipartiteGraph *bip = new BipartiteGraph();
    unsigned short depth = 0;
    
    // Bipartite is only considered at the first internal node with more than one child.
    NetworkTreeNode *entryPoint = root;
    while(entryPoint->getChildren()->size() == 1)
    {
        entryPoint = entryPoint->getChildren()->front();
        depth++;
    }
    
    // If the node is an Hedera, we will start from the first parent which is not one.
    while(entryPoint->isHedera())
    {
        entryPoint = entryPoint->getParent();
        depth--;
    }
    
    // Recursively visits the tree to build the bipartite graph.
    toBipartiteRecursive(bip, this, entryPoint, depth);
    
    return bip;
}

NetworkTreeNode *NetworkTree::insertSubnet(SubnetSite *subnet, unsigned short depth)
{
    InetAddress *route = subnet->getRoute();
    unsigned short routeSize = subnet->getRouteSize();
    
    // If current depth minus 1 equals the route size, then we just have to create a leaf
    if((depth - 1) == routeSize)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(subnet);
        return newNode;
    }

    // Creates the branch for this new subnet, as a cascade of new nodes
    NetworkTreeNode *newRoot = new NetworkTreeNode(route[depth - 1]);
    NetworkTreeNode *curNode = newRoot;
    for(int i = depth + 1; i <= routeSize; i++)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(route[i - 1]);
        newNode->addPreviousLabel(route[i - 2]);
        curNode->addChild(newNode);
        curNode = newNode;
    }
    curNode->addChild(new NetworkTreeNode(subnet));
    
    return newRoot;
}

void NetworkTree::prune(list<NetworkTreeNode*> *map, 
                        NetworkTreeNode *cur, 
                        NetworkTreeNode *prev,
                        unsigned short depth)
{
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
        prune(map, cur->getParent(), cur, depth - 1);
    }
    // Current node has no longer children. Moves up in the tree.
    else if(children->size() == 0)
    {
        prune(map, cur->getParent(), cur, depth - 1);
    }
}

void NetworkTree::visitRecursive(ostream *out, NetworkTreeNode *cur, unsigned short depth)
{
    (*out) << depth << " - ";
    
    // Displays root node
    if(cur->isRoot())
    {
        (*out) << "Root node" << endl;
        
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            visitRecursive(out, (*i), depth + 1);
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
            visitRecursive(out, (*i), depth + 1);
        }
    }
}

void NetworkTree::statisticsRecursive(unsigned int *stat, NetworkTree *tree, NetworkTreeNode *cur)
{
    stat[0]++;
    
    if(cur->hasOnlyLeavesAsChildren())
    {
        stat[1]++;
        stat[2]++;
        stat[3]++;
    }
    else
    {
        unsigned short linkage = cur->getLinkage();
        if(linkage == 0)
        {
            stat[2]++;
            stat[3]++;
        }
        else if(linkage == 1)
        {
            stat[3]++;
        }
    }
    
    // Checks how many nodes have their labels all appearing in inferred subnets
    list<InetAddress> *labels = cur->getLabels();
    unsigned short nbLabels = labels->size();
    unsigned short appearingLabels = 0;
    for(list<InetAddress>::iterator l = labels->begin(); l != labels->end(); ++l)
    {
        InetAddress curLabel = (*l);
        SubnetSite *container = tree->getSubnetContaining(curLabel);
        if(container != NULL)
            appearingLabels++;
    }
    if(appearingLabels == nbLabels)
    {
        stat[4]++;
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->isInternal())
        {
            statisticsRecursive(stat, tree, (*i));
        }
    }
}

void NetworkTree::collectHintsRecursive(ostream *out, 
                                        AliasHintCollector *ahc, 
                                        NetworkTreeNode *cur,
                                        unsigned short depth)
{
    // Root: goes deeper
    if(cur->isRoot())
    {
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            collectHintsRecursive(out, ahc, (*i), depth + 1);
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
            
            (*out) << "... " << endl;
            
            ahc->setIPsToProbe(interfacesToProbe);
            ahc->setCurrentTTL((unsigned char) depth);
            ahc->collect();
            
            // Small delay before analyzing next internal (typically quarter of a second)
            Thread::invokeSleep(ahc->getEnvironment()->getProbeThreadDelay());
        }
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            collectHintsRecursive(out, ahc, (*i), depth + 1);
        }
    }
}

void NetworkTree::inferRoutersRecursive(NetworkTree *tree, 
                                        NetworkTreeNode *cur, 
                                        AliasResolver *ar, 
                                        unsigned short depth)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    
    // Puts direct neighbor subnets in a list and puts the internal nodes in another one
    list<NetworkTreeNode*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        // Just in case (PlanetLab...)
        if((*i) == NULL)
            continue;
    
        if((*i)->isLeaf())
            childrenL.push_back((*i));
        else if((*i)->isInternal())
            childrenI.push_back((*i));
    }
    
    size_t nbNeighborSubnets = childrenL.size();

    // Router inference
    if(nbNeighborSubnets > 0)
    {
        ar->setCurrentTTL(depth);
        ar->resolve(cur);
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i) != NULL && (*i)->isInternal())
            inferRoutersRecursive(tree, (*i), ar, depth + 1);
    }
}

void NetworkTree::largestListRecursive(NetworkTreeNode *cur, unsigned int *largest)
{
    // Checks the size
    unsigned int curSize = (unsigned int) cur->getFingerprints().size();
    if(curSize > (*largest))
    {
        (*largest) = curSize;
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->isInternal())
        {
            largestListRecursive((*i), largest);
        }
    }
}

void NetworkTree::internalsRecursive(ostream *out, NetworkTree *tree, NetworkTreeNode *cur)
{
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
        
        (*out) << "Inferred a total " << egressInterfaces + ingressInterfaces << " interfaces." 
        << endl;
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
        
        (*out) << endl << "Label analysis:" << endl;
        unsigned short nbLabels = labels->size();
        unsigned short appearingLabels = 0;
        for(list<InetAddress>::iterator l = labels->begin(); l != labels->end(); ++l)
        {
            InetAddress curLabel = (*l);
            SubnetSite *container = tree->getSubnetContaining(curLabel);
            if(container != NULL)
            {
                (*out) << "Label " << curLabel << " belongs to registered subnet ";
                (*out) << container->getInferredNetworkAddressString() << endl;
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
        
        // Displays sorted fingerprints if any
        list<Fingerprint> fingerprints = cur->getFingerprints();
        if(fingerprints.size() > 0)
        {
            (*out) << "\nFingerprints (sorted):\n";
            for(list<Fingerprint>::iterator it = fingerprints.begin(); it != fingerprints.end(); ++it)
            {
                (*out) << (InetAddress) (*((*it).ipEntry)) << " - " << (*it) << endl;
            }
        }
        
        // Router inference
        list<Router*> *routers = cur->getInferredRouters();
        unsigned short nbRouters = (unsigned short) routers->size();
        if(nbRouters > 0)
        {
            (*out) << endl;
            if(nbRouters == 1)
                (*out) << "Inferred router: " << endl;
            else
                (*out) << "Inferred routers: " << endl;
            
            for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
                (*out) << (*i)->toString() << endl;
        }
        else
        {
            (*out) << "Inference not done, or data is lacking for credible router inference." << endl;
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
        (*out) << "No neighboring subnets, but inferred a total of " 
        << egressInterfaces + ingressInterfaces << " interfaces." << endl;
        unsigned short linkage = cur->getLinkage();
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
                SubnetSite *container = tree->getSubnetContaining(curLabel);
                if(container != NULL)
                {
                    (*out) << "Label " << curLabel << " belongs to registered subnet ";
                    (*out) << container->getInferredNetworkAddressString() << endl;
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
            internalsRecursive(out, tree, (*i));
    }
}

void NetworkTree::listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur)
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

void NetworkTree::outputAliasesRecursive(NetworkTreeNode *cur, string *aliasesStr)
{
    // Prints aliases
    if(!cur->isRoot())
    {
        list<Router*> *routers = cur->getInferredRouters();
        unsigned short nbRouters = (unsigned short) routers->size();
        if(nbRouters > 0)
        {
            for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
            {
                (*aliasesStr) += (*i)->toStringBis() + "\n";
            }
        }
    }

    // Goes deeper in the tree
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if(!(*i)->isLeaf())
            outputAliasesRecursive((*i), aliasesStr);
    }
}

void NetworkTree::listInterfacesRecursive(list<InetAddress> *interfaces, NetworkTreeNode *cur)
{
    // Stops if it is a leaf
    if(cur->isLeaf())
        return;

    // Goes through children and finds internals among them
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        NetworkTreeNode *curChild = (*i);
        if(curChild->isInternal())
        {
            list<InetAddress> *labels = curChild->getLabels();
            for(list<InetAddress>::iterator j = labels->begin(); j != labels->end(); ++j)
            {
                InetAddress interface = (*j);
                if(interface != InetAddress("0.0.0.0"))
                {
                    interfaces->push_back(interface);
                }
            }
            
            // Goes deeper
            listInterfacesRecursive(interfaces, (*i));
        }
    }
}

void NetworkTree::nullifyLeavesRecursive(SubnetSiteSet *sink, NetworkTreeNode *cur)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        NetworkTreeNode *cur = (*i);
        if(cur->isLeaf())
            cur->nullifySubnet(sink);
        else
            nullifyLeavesRecursive(sink, cur);
    }
}

void NetworkTree::toBipartiteRecursive(BipartiteGraph *bip, 
                                       NetworkTree *tree,
                                       NetworkTreeNode *cur,
                                       unsigned short depth)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    list<Router*> *routers = cur->getInferredRouters();
    
    // Puts direct neighbor subnets in a list and puts the T_NEIGHBORHOOD nodes in another one
    list<SubnetSite*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->isLeaf())
        {
            childrenL.push_back((*i)->getAssociatedSubnet());
        }
        else if((*i)->isInternal())
        {
            childrenI.push_back((*i));
        }
    }
    
    // Goes deeper in the tree first
    for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
    {
        toBipartiteRecursive(bip, tree, (*i), depth + 1);
    }
    
    // Case where cur has multiple labels (i.e. an Hedera)
    if(cur->isHedera())
    {
        for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
        {
            InetAddress ingressInterface = (*i);
            
            // Skips 0.0.0.0 label if it exists (will be replaced by an imaginary router)
            if(ingressInterface == InetAddress("0"))
                continue;
            
            Router *trueIngressRouter = cur->getRouterHaving(ingressInterface);
            
            // If no router for this label, skips too (an imaginary router will replace it)
            if(trueIngressRouter == NULL)
                continue;
            
            // Lists children subnets which last label is ingressInterface
            list<SubnetSite*> curChildrenL;
            for(list<SubnetSite*>::iterator j = childrenL.begin(); j != childrenL.end(); ++j)
            {
                if((*j)->hasRouteLabel(ingressInterface, depth))
                {
                    curChildrenL.push_back((*j));
                    childrenL.erase(j--);
                }
            }
            list<SubnetSite*> backUpSubnets(curChildrenL);
            
            /*
             * Lists children internals which the routes to the leaves contain at least one 
             * occurrence of ingressInterface.
             */
            
            list<NetworkTreeNode*> curChildrenI;
            for(list<NetworkTreeNode*>::iterator j = childrenI.begin(); j != childrenI.end(); ++j)
            {
                if((*j)->hasPreviousLabel(ingressInterface))
                {
                    curChildrenI.push_back((*j));
                    
                    // No erasure, because this node might be accessed by another router
                    
                    break;
                }
            }
            
            /*
             * N.B.: the code below, creating the bipartite router and connecting it to the listed 
             * subnets, also ensures that we do not create a duplicate router (this can occur if 
             * a router features several labels of the node as interfaces). However, since the 
             * router will probably be not connected to all subnets for which the current label 
             * appears on their route, we do not skip this iteration.
             */
            
            if(!trueIngressRouter->hasBipEquivalent())
                bip->addRouter(trueIngressRouter);
            
            bipConnectWithSubnets(bip, trueIngressRouter, &curChildrenL);
            
            // Gets ingress router as a bipartite element
            BipartiteRouter *ingressRouter = trueIngressRouter->getBipEquivalent();
            
            /*
             * The code now looks for routers which give access to the remaining subnets. It will 
             * connect these subnets with the selected routers, then connect the routers with an 
             * Ethernet switch to the ingress router for this label.
             */
            
            BipartiteSwitch *connectingSwitch = NULL;
            if(curChildrenL.size() > 0 && routers->size() > 1)
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    if((*j)->getBipEquivalent() == ingressRouter)
                        continue;
                    
                    // Router must give access to at least one listed subnet to continue
                    list<SubnetSite*>::iterator listBegin = curChildrenL.begin();
                    list<SubnetSite*>::iterator listEnd = curChildrenL.end();
                    bool givesAccessToASubnet = false;
                    for(list<SubnetSite*>::iterator k = listBegin; k != listEnd; ++k)
                    {
                        if((*j)->givesAccessTo((*k)))
                        {
                            givesAccessToASubnet = true;
                            break;
                        }
                    }
                    
                    // Next iteration if no match with the listed subnets
                    if(!givesAccessToASubnet)
                        continue;
                    
                    if(!(*j)->hasBipEquivalent())
                        bip->addRouter((*j));
                    bipConnectWithSubnets(bip, (*j), &curChildrenL);
                    
                    // Connects to the ingress router via an "imaginary" switch
                    if(connectingSwitch == NULL)
                    {
                        connectingSwitch = bip->createSwitch();
                        bip->createLinkSR(connectingSwitch, ingressRouter);
                    }
                    bip->createLinkSR(connectingSwitch, (*j)->getBipEquivalent());
                }
            }
            
            // If there remains subnets, connects them with imaginary router
            if(curChildrenL.size() > 0)
            {
                BipartiteRouter *imaginary = cur->getImaginaryRouter();
                if(imaginary == NULL)
                {
                    imaginary = bip->createImaginaryRouter();
                    cur->setImaginaryRouter(imaginary);
                }
            
                list<SubnetSite*>::iterator listBegin = curChildrenL.begin();
                list<SubnetSite*>::iterator listEnd = curChildrenL.end();
                for(list<SubnetSite*>::iterator j = listBegin; j != listEnd; ++j)
                {
                    if(connectingSwitch == NULL)
                    {
                        connectingSwitch = bip->createSwitch();
                        bip->createLinkSR(connectingSwitch, ingressRouter);
                    }
                    
                    if(!(*j)->hasBipEquivalent())
                        bip->addSubnet((*j));
                    
                    bip->createLinkSR(connectingSwitch, imaginary);
                    bip->createLinkRS(imaginary, (*j)->getBipEquivalent());
                }
                curChildrenL.clear();
            }
            
            // Connects router with children that are internals
            if(curChildrenI.size() > 0)
            {
                bipConnectWithInternals(bip, 
                                        tree, 
                                        ingressRouter, 
                                        &backUpSubnets, 
                                        &curChildrenI);
            }
        }
    }
    // (simpler) Case where there is a single interface and at least one router
    else if(routers->size() > 0)
    {
        list<SubnetSite*> backUpSubnets(childrenL); // Copy of childrenL for last step
        
        InetAddress ingressInterface = labels->front();
        BipartiteRouter *ingressRouter = NULL;
        
        // Creates an imaginary router if label is 0.0.0.0
        if(ingressInterface == InetAddress(0))
        {
            ingressRouter = cur->getImaginaryRouter();
            if(ingressRouter == NULL)
            {
                ingressRouter = bip->createImaginaryRouter();
                cur->setImaginaryRouter(ingressRouter);
            }
        }
        // Otherwise, gets corresponding router, adds it in bipartite and connects with subnets
        else
        {
            Router *trueIngressRouter = cur->getRouterHaving(ingressInterface);
            if(!trueIngressRouter->hasBipEquivalent())
                bip->addRouter(trueIngressRouter);
            bipConnectWithSubnets(bip, trueIngressRouter, &childrenL);
            ingressRouter = trueIngressRouter->getBipEquivalent();
        }
        
        // Code proceeds with other routers in the neighborhood (if any)
        BipartiteSwitch *connectingSwitch = NULL;
        if(routers->size() > 1)
        {
            for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
            {
                if((*i)->getBipEquivalent() == ingressRouter)
                    continue;
                
                if(!(*i)->hasBipEquivalent())
                    bip->addRouter((*i));
                bipConnectWithSubnets(bip, (*i), &childrenL);
                
                // Connects to the ingress router via an "imaginary" switch
                if(connectingSwitch == NULL)
                {
                    connectingSwitch = bip->createSwitch();
                    bip->createLinkSR(connectingSwitch, ingressRouter);
                }
                bip->createLinkSR(connectingSwitch, (*i)->getBipEquivalent());
            }
        }
        
        // If there remains subnets, creates an imaginary router for every of them
        if(childrenL.size() > 0)
        {
            BipartiteRouter *imaginary = cur->getImaginaryRouter();
            if(imaginary == NULL)
            {
                imaginary = bip->createImaginaryRouter();
                cur->setImaginaryRouter(imaginary);
            }
        
            for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
            {
                if(connectingSwitch == NULL)
                {
                    connectingSwitch = bip->createSwitch();
                    bip->createLinkSR(connectingSwitch, ingressRouter);
                }
                
                if(!(*i)->hasBipEquivalent())
                    bip->addSubnet((*i));
                
                bip->createLinkSR(connectingSwitch, imaginary);
                bip->createLinkRS(imaginary, (*i)->getBipEquivalent());
            }
            childrenL.clear();
        }
        
        // Connects router with children that are internals
        if(childrenI.size() > 0)
        {
            bipConnectWithInternals(bip, 
                                    tree, 
                                    ingressRouter, 
                                    &backUpSubnets, 
                                    &childrenI);
        }
    }
    // Case where there is no router at all
    else
    {
        // Creates an imaginary router for the whole neighborhood
        BipartiteRouter* ingressRouter = cur->getImaginaryRouter();
        if(ingressRouter == NULL)
        {
            ingressRouter = bip->createImaginaryRouter();
            cur->setImaginaryRouter(ingressRouter);
        }
        
        // Connects all subnets to it
        for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
        {
            if(!(*i)->hasBipEquivalent())
                bip->addSubnet((*i));
            
            bip->createLinkRS(ingressRouter, (*i)->getBipEquivalent());
        }
        
        // Connects imaginary router with children that are internals
        if(childrenI.size() > 0)
        {
            bipConnectWithInternals(bip, 
                                    tree, 
                                    ingressRouter, 
                                    &childrenL, 
                                    &childrenI);
        }
    }
}

void NetworkTree::bipConnectWithSubnets(BipartiteGraph *bip,
                                        Router *ingressRouter,
                                        list<SubnetSite*> *childrenL)
{
    BipartiteRouter *bipIngress = ingressRouter->getBipEquivalent();
    for(list<SubnetSite*>::iterator i = childrenL->begin(); i != childrenL->end(); ++i)
    {
        /*
         * Checks there is not already a link between ingressRouter and a child subnet. In very 
         * specific situations involving Hedera's, it is possible the ingress router is already 
         * connected to one subnet if this subnet is a crossed subnet between a previously 
         * considered internal node and ingress router (previous label). It is however quite rare, 
         * but it is still worth a check.
         */

        BipartiteSubnet *bipSubnet = NULL;
        if((*i)->hasBipEquivalent())
        {
            bipSubnet = (*i)->getBipEquivalent();
            if(bipIngress != NULL && bipIngress->isConnectedToSubnet(bipSubnet->getLabel()))
                continue;
        }
    
        // Regular code creating links
        if(ingressRouter->givesAccessTo((*i)))
        {
            if(bipSubnet == NULL)
                bip->addSubnet((*i));
            
            bip->createLink(ingressRouter, (*i));
            childrenL->erase(i--);
        }
    }
}

void NetworkTree::bipConnectWithInternals(BipartiteGraph *bip,
                                          NetworkTree *tree,
                                          BipartiteRouter *ingressRouter,
                                          list<SubnetSite*> *childrenL,
                                          list<NetworkTreeNode*> *childrenI)
{
    for(list<NetworkTreeNode*>::iterator i = childrenI->begin(); i != childrenI->end(); ++i)
    {
        list<InetAddress> *labelsChild = (*i)->getLabels();
        
        // Internal is not a load balancing node
        if(labelsChild->size() == 1)
        {
            InetAddress labelChild = labelsChild->front();
            
            bipConnectWithInternal(bip,
                                   tree,
                                   ingressRouter,
                                   childrenL,
                                   (*i), 
                                   labelChild, 
                                   (*i)->getRouterHaving(labelChild));
        }
        // Creates a link (with or without imaginary subnet) for each router containing a label
        else
        {
            list<InetAddress> copyLabels;
            for(list<InetAddress>::iterator j = labelsChild->begin(); j != labelsChild->end(); ++j)
                copyLabels.push_back(InetAddress((*j)));
            
            for(list<InetAddress>::iterator j = copyLabels.begin(); j != copyLabels.end(); ++j)
            {
                Router *ingressRouterChild = (*i)->getRouterHaving((*j));
                
                /*
                 * If the router exists, removes from copyLabels the label different from 
                 * ingressInterface which also happens to be an interface of the router from 
                 * the current child internal node.
                 */
                
                if(ingressRouterChild != NULL)
                {
                    bool guardian = true; // Avoids deleting first label
                    for(list<InetAddress>::iterator k = j; k != copyLabels.end(); ++k)
                    {
                        if(guardian)
                        {
                            guardian = false;
                            continue;
                        }
                        
                        if(ingressRouterChild->hasInterface((*k)))
                        {
                            copyLabels.erase(k--);
                        }
                    }
                }
                
                bipConnectWithInternal(bip,
                                       tree,
                                       ingressRouter,
                                       childrenL, 
                                       (*i), 
                                       (*j), 
                                       ingressRouterChild);
            }
        }
    }
}

void NetworkTree::bipConnectWithInternal(BipartiteGraph *bip,
                                         NetworkTree *tree,
                                         BipartiteRouter *ingressRouter,
                                         list<SubnetSite*> *childrenL,
                                         NetworkTreeNode *childN,
                                         InetAddress labelChild,
                                         Router* ingressRouterChild)
{
    bool successfullyConnected = false;
        
    /*
     * Tries to find the subnet being crossed to reach this internal and connects this 
     * subnet with the router of the internal which has the label among its interfaces.
     */
    
    for(list<SubnetSite*>::iterator i = childrenL->begin(); i != childrenL->end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        // Checks there is not already a link (can happen with Hedera's)
        if(ingressRouterChild != NULL)
        {
            BipartiteRouter *bipIngress = ingressRouterChild->getBipEquivalent();
            string label = "";
            if(ss->getBipEquivalent() != NULL)
                label = ss->getBipEquivalent()->getLabel();

            if(label.length() > 0 && bipIngress != NULL && bipIngress->isConnectedToSubnet(label))
            {
                successfullyConnected = true;
                break;
            }
        }
        
        // Links does not exist, creates it if a subnet contains the label of child internal
        if(ss->contains(labelChild))
        {
            if(ingressRouterChild == NULL)
            {
                BipartiteSubnet *bipSubnet = ss->getBipEquivalent();
                BipartiteRouter *bipRouter = childN->getImaginaryRouter();
                
                /* 
                 * Link might already exists if the neighborhood containing bipSubnet was 
                 * previously visited. Having a duplicate link is rare, but it could occur with 
                 * internal nodes that have no associated routers but which label belongs to some 
                 * ODD/SHADOW subnet which would be bipSubnet here. Due to previous operations, 
                 * the link might already exists.
                 */
                
                if(bipRouter == NULL)
                {
                    bipRouter = bip->createImaginaryRouter();
                    childN->setImaginaryRouter(bipRouter);
                    bip->createLinkRS(bipRouter, bipSubnet);
                }
                else if(!bipRouter->isConnectedToSubnet(bipSubnet->getLabel()))
                {
                    bip->createLinkRS(bipRouter, bipSubnet);
                }
            }
            else
            {
                bip->createLink(ingressRouterChild, ss);
            }
            successfullyConnected = true;
            break;
        }
    }
    
    // Instructions for when the crossed subnet could not be found.
    if(!successfullyConnected)
    {
        /* 
         * First we check if a subnet containing this label does not appear somewhere 
         * else in the tree (this can occur due to route stretching). Additionnaly, this 
         * subnet should NOT be a child of the child internal node to use it (it should 
         * be connected to the child internal bipartite equivalent, and the child internal 
         * will be connected with an imaginary subnet).
         */
        
        SubnetSite *connectingSubnet = tree->getSubnetContaining(labelChild);
        bool notAChildSubnet = true;
        list<NetworkTreeNode*> *children = childN->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            if((*i)->getAssociatedSubnet() == connectingSubnet)
            {
                notAChildSubnet = false;
                break;
            }
        }
        
        // If a connecting subnet exists
        if(connectingSubnet != NULL && notAChildSubnet)
        {
            // Creates a link between ingress of this node and this subnet
            if(!connectingSubnet->hasBipEquivalent())
                bip->addSubnet(connectingSubnet);
            
            BipartiteSubnet *bipSubnet = connectingSubnet->getBipEquivalent();
        
            // Creates the link with ingress if it does not exist yet
            string bipSubnetLabel = bipSubnet->getLabel();
            if(!ingressRouter->isConnectedToSubnet(bipSubnetLabel))
                bip->createLinkRS(ingressRouter, bipSubnet);

            // Gets the ingress router of this child node and connects with subnet
            if(ingressRouterChild == NULL)
            {
                BipartiteRouter *bipRouter = childN->getImaginaryRouter();
                
                /* 
                 * Link might already exists if the neighborhood containing bipSubnet was 
                 * previously visited.
                 */
                
                if(bipRouter == NULL)
                {
                    bipRouter = bip->createImaginaryRouter();
                    childN->setImaginaryRouter(bipRouter);
                    bip->createLinkRS(bipRouter, bipSubnet);
                }
                else if(!bipRouter->isConnectedToSubnet(bipSubnet->getLabel()))
                {
                    bip->createLinkRS(bipRouter, bipSubnet);
                }
            }
            else
            {
                /*
                 * Checks a link does not already exist, because duplicate links can occur with 
                 * an Hedera which several labels belongs to a same router and which belongs to 
                 * remote connecting subnets.
                 */
                
                BipartiteRouter *bipRouter = ingressRouterChild->getBipEquivalent();
                if(bipRouter != NULL && !bipRouter->isConnectedToSubnet(bipSubnet->getLabel()))
                {
                    bip->createLink(ingressRouterChild, connectingSubnet);
                }
            }
        }
        // Otherwise: one needs to create an imaginary subnet
        else
        {
            BipartiteSubnet *bipSubnet = bip->createImaginarySubnet();
            bip->createLinkRS(ingressRouter, bipSubnet);
            
            // Gets the ingress router of this node and connects with bipSubnet
            if(ingressRouterChild == NULL)
            {
                BipartiteRouter *bipRouter = childN->getImaginaryRouter();
                bip->createLinkRS(bipRouter, bipSubnet);
            }
            else
            {
                BipartiteRouter *bipRouter = ingressRouterChild->getBipEquivalent();
                bip->createLinkRS(bipRouter, bipSubnet);
            }
        }
    }
}
