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
#include "../../common/thread/Thread.h" // For invokeSleep()

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
    InetAddress *route = subnet->getRefinementRoute();
    unsigned short routeSize = subnet->getRefinementRouteSize();
    
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

    // Puts the new internal nodes inside the depth map
    NetworkTreeNode *next = subTree;
    if(next->isInternal())
    {
        unsigned short curDepth = insertionPointDepth;
        do
        {
            this->depthMap[curDepth].push_back(next);
            curDepth++;
            
            list<NetworkTreeNode*> *children = next->getChildren();
            if(children != NULL && children->size() == 1)
            {
                if(children->front()->isInternal())
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
             * to be fidel to the topology.
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
    InetAddress baseIP = subnet->getInferredNetworkAddress().getLowerBorderAddress();
    unsigned long index = (baseIP.getULongAddress() >> 12);
    this->subnetMap[index].push_back(subnet);
}

void NetworkTree::visit(ostream *out)
{
    visitRecursive(out, root, 0);
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
        if(ss->containsAddress(needle))
            return ss;
    }
    return NULL;
}

void NetworkTree::internals(ostream *out, AliasResolver *ar)
{
    internalsRecursive(out, this, root, ar);
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
        string cur = ss->refinedToString();
        
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

NetworkTreeNode *NetworkTree::insertSubnet(SubnetSite *subnet, unsigned short depth)
{
    InetAddress *route = subnet->getRefinementRoute();
    unsigned short routeSize = subnet->getRefinementRouteSize();
    
    // If current depth minus 1 equals the route size, then we just have to create a leaf
    if((depth - 1) == routeSize)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(subnet);
        return newNode;
    }

    // Creates the branch for this new subnet, as a cascade of new nodes with missing interfaces
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

void NetworkTree::repairRoute(SubnetSite *ss)
{
    // Finds deepest match in the tree
    InetAddress *route = ss->getRefinementRoute();
    unsigned short routeSize = ss->getRefinementRouteSize();
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
        
        InetAddress *curRoute = cur->getRefinementRoute();
        
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
    bool *repairMask = new bool[routeSize];
    for(unsigned short i = 0; i < routeSize; ++i)
        repairMask[i] = false;
    
    for(unsigned short i = 0; i < insertionPointDepth; ++i)
    {
        if(route[i] == InetAddress("0.0.0.0"))
        {
            route[i] = similarRoute[i];
            repairMask[i] = true;
        }
    }
    
    ss->setRouteRepairMask(repairMask);
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
            cout << "}";
        }
        else
        {
            (*out) << "Neighborhood {" << cur->getLabels()->front() << "}";
        }
        
        (*out) << "... ";
        
        ahc->setIPsToProbe(cur->listInterfaces());
        ahc->setCurrentTTL((unsigned char) depth);
        ahc->collect();
        
        (*out) << "Done." << endl;
        
        // Small delay before analyzing next internal (typically half a second)
        Thread::invokeSleep(ahc->getEnvironment()->getProbeThreadDelay() * 2);
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            collectHintsRecursive(out, ahc, (*i), depth + 1);
        }
    }
}

void NetworkTree::internalsRecursive(ostream *out, 
                                     NetworkTree *tree, 
                                     NetworkTreeNode *cur,
                                     AliasResolver *ar)
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
    unsigned int egressInterfaces = (unsigned int) nbChildren;
    
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
        }
        else if((*i)->isInternal())
        {
            childrenI.push_back((*i));
        }
    }
    
    size_t nbNeighborSubnets = childrenL.size();
    unsigned short countchildrenI = (unsigned short) childrenI.size();

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
            unsigned short status = ss->getRefinementStatus();
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
                        if(ss->containsAddress((*k)))
                        {
                            onTheWay.push_back((*k));
                            countchildrenI--;
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
        if(countchildrenI == 0)
        {
            (*out) << "This internal node has complete linkage with its children." << endl;
        }
        else if(countchildrenI == 1)
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
        
        // Router inference
        list<Router> routers = ar->resolve(cur);
        unsigned short nbRouters = (unsigned short) routers.size();
        if(nbRouters > 0)
        {
            (*out) << endl;
            if(nbRouters == 1)
                (*out) << "Inferred router: " << endl;
            else
                (*out) << "Inferred routers: " << endl;
            
            for(list<Router>::iterator i = routers.begin(); i != routers.end(); ++i)
            {
                Router cur = (*i);
                list<RouterInterface> *IPs = cur.getInterfacesList();
                (*out) << "[";
                bool first = true;
                for(list<RouterInterface>::iterator j = IPs->begin(); j != IPs->end(); ++j)
                {
                    if(!first)
                        (*out) << ", ";
                    else
                        first = false;
                    (*out) << (*j).ip;
                    
                    // Precise alias resolution method
                    switch((*j).aliasMethod)
                    {
                        case RouterInterface::ALLY:
                            (*out) << " (Ally)";
                            break;
                        case RouterInterface::IPID_VELOCITY:
                            (*out) << " (Velocity)";
                            break;
                        case RouterInterface::REVERSE_DNS:
                            (*out) << " (Reverse DNS)";
                            break;
                        default:
                            break;
                    }
                }
                (*out) << "]" << endl;
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
            internalsRecursive(out, tree, (*i), ar);
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
