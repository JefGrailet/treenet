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
#include "../bipartite/BipartiteSwitch.h"
#include "../bipartite/BipartiteRouter.h"
#include "../bipartite/BipartiteSubnet.h"

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

list<InetAddress*> NetworkTree::listInterfaces()
{
    list<InetAddress*> list;
    listInterfacesRecursive(&list, root);
    return list;
}

void NetworkTree::propagateRouterInfo()
{
    propagateRecursive(root);
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

unsigned int *NetworkTree::getStatistics()
{
    unsigned int *stat = new unsigned int[5];
    for(unsigned int i = 0; i < 5; i++)
        stat[i] = 0;
    
    statisticsRecursive(stat, this, root);
    
    return stat;
}

void NetworkTree::visitAndInferRouters()
{
    visitAndInferRoutersRecursive(root);
}

void NetworkTree::neighborhoods(ostream *out)
{
    neighborhoodsRecursive(out, this, root);
}

void NetworkTree::outputAsFile(string filename)
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
    
    /*
     * If the node is a load balancer, we will start from the first parent which is not a
     * load balancer.
     */
    
    while(entryPoint->isLoadBalancer())
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
    if(cur->getType() == NetworkTreeNode::T_SUBNET)
    {
        if(prev != NULL)
            delete prev;
        return;
    }
    else if(cur->getType() == NetworkTreeNode::T_ROOT)
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
    unsigned short type = cur->getType();
    
    (*out) << depth << " - ";
    
    // Displays root node
    if(type == NetworkTreeNode::T_ROOT)
    {
        (*out) << "Root node" << endl;
        
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            visitRecursive(out, (*i), depth + 1);
        }
    }
    // Displays a leaf (subnet)
    else if(type == NetworkTreeNode::T_SUBNET)
    {
        (*out) << "Subnet: " << cur->getAssociatedSubnet()->getInferredNetworkAddressString() << endl;
    }
    // Any other case: internal node (with or without load balancing)
    else
    {
        if(cur->isLoadBalancer())
        {
            (*out) << "Internal (load balancer): ";
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
            (*out) << "Internal: " << cur->getLabels()->front();
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
    
    if(cur->hasLeavesAsChildren())
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
        if((*i)->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
        {
            statisticsRecursive(stat, tree, (*i));
        }
    }
}

void NetworkTree::listInterfacesRecursive(list<InetAddress*> *interfacesList, NetworkTreeNode *cur)
{
    unsigned short type = cur->getType();

    // Leaves are irrelevant
    if(type == NetworkTreeNode::T_SUBNET)
    {
        return;
    }
    // Add the interfaces of this neighborhood
    if(type == NetworkTreeNode::T_NEIGHBORHOOD)
    {
        list<InetAddress*> list2 = cur->listInterfaces();
        for(list<InetAddress*>::iterator i = list2.begin(); i != list2.end(); ++i)
        {
            // Also resets alias info before putting the IP in the list
            (*i)->setProbeToken(0);
            (*i)->setIPIdentifier(0);
            (*i)->setStoredHostName("");
            interfacesList->push_back((*i));
        }
    }
    
    // Goes deeper in the tree
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->getType() != NetworkTreeNode::T_SUBNET)
            listInterfacesRecursive(interfacesList, (*i));
    }
}

void NetworkTree::propagateRecursive(NetworkTreeNode *cur)
{
    unsigned short type = cur->getType();

    // Storing subnets in leaves
    if(type == NetworkTreeNode::T_SUBNET)
    {
        cur->copyRouterInfoIntoRoute();
        return;
    }
    
    // Goes deeper in the tree
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        propagateRecursive((*i));
    }
}

void NetworkTree::visitAndInferRoutersRecursive(NetworkTreeNode *cur)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    unsigned short nbChildren = (unsigned short) children->size();
    
    // Counts direct neighbor subnets and puts the T_NEIGHBORHOOD nodes in a list
    unsigned short nbNeighborSubnets = 0;
    list<NetworkTreeNode*> childrenN;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->getType() == NetworkTreeNode::T_SUBNET)
        {
            nbNeighborSubnets++;
        }
        else if((*i)->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
        {
            childrenN.push_back((*i));
        }
    }

    if(nbNeighborSubnets > 0 || nbChildren > 1 || cur->isLoadBalancer())
    {
        cur->inferRouters();
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    for(list<NetworkTreeNode*>::iterator i = childrenN.begin(); i != childrenN.end(); ++i)
    {
        visitAndInferRoutersRecursive((*i));
    }
}

void NetworkTree::neighborhoodsRecursive(ostream *out, NetworkTree *tree, NetworkTreeNode *cur)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    size_t nbChildren = children->size();
    
    // Ingress/egress interfaces count (for amount of interfaces inference)
    unsigned int ingressInterfaces = 1;
    NetworkTreeNode *parent = cur->getParent();
    if(parent != NULL && parent->getType() != NetworkTreeNode::T_ROOT)
    {
        ingressInterfaces = parent->getLabels()->size();
    }
    unsigned int egressInterfaces = (unsigned int) nbChildren;
    
    // Puts direct neighbor subnets in a list and puts the T_NEIGHBORHOOD nodes in another one
    list<NetworkTreeNode*> childrenS;
    list<NetworkTreeNode*> childrenN;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->getType() == NetworkTreeNode::T_SUBNET)
        {
            childrenS.push_back((*i));
        }
        else if((*i)->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
        {
            childrenN.push_back((*i));
        }
    }
    
    size_t nbNeighborSubnets = childrenS.size();
    unsigned short countChildrenN = (unsigned short) childrenN.size();

    if(nbNeighborSubnets > 0)
    {
        if(cur->getType() == NetworkTreeNode::T_ROOT)
        {
            (*out) << "Neighborhood of this computer:" << endl;
        }
        else
        {
            if (cur->isLoadBalancer())
            {
                (*out) << "Neighborhood of load balancer {";
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
        
        for(list<NetworkTreeNode*>::iterator i = childrenS.begin(); i != childrenS.end(); ++i)
        {
            SubnetSite *ss = (*i)->getAssociatedSubnet();
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
            
            if(childrenN.size() > 0)
            {
                list<InetAddress> onTheWay;
                for(list<NetworkTreeNode*>::iterator j = childrenN.begin(); j != childrenN.end(); ++j)
                {
                    list<InetAddress> *labels = (*j)->getLabels();
                    
                    for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                    {
                        if(ss->contains((*k)))
                        {
                            onTheWay.push_back((*k));
                            countChildrenN--;
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
        if(countChildrenN == 0)
        {
            (*out) << "This neighborhood has complete linkage with its children." << endl;
        }
        else if(countChildrenN == 1)
        {
            (*out) << "This neighborhood has partial linkage with its children." << endl;
        }
        
        /*
         * The code now checks that each label of the neighborhood belongs to a subnet which is 
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
        
        // Inferred routers
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
            {
                Router *cur = (*i);
                string curStr = cur->toString();
                (*out) << curStr << endl;
            }
        }
        else
        {
            (*out) << "Data is lacking for credible router inference." << endl;
        }
        
        (*out) << endl;
        
        (*out) << "------------------------------------------" << endl << endl;
    }
    // More than 2 children or being a load balancer needed to legitimate display
    else if(nbChildren > 1 || cur->isLoadBalancer())
    {
        if(cur->getType() == NetworkTreeNode::T_ROOT)
        {
            (*out) << "Neighborhood of this computer:" << endl;
        }
        else
        {
            if(cur->isLoadBalancer())
            {
                (*out) << "Neighborhood of load balancer {";
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
        
        /*
         * The code now checks that each label of the neighborhood belongs to a subnet which is 
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
        
        // Inferred routers
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
            {
                Router *cur = (*i);
                string curStr = cur->toString();
                (*out) << curStr << endl;
            }
        }
        else
        {
            (*out) << "Data is lacking for credible router inference." << endl;
        }
        
        (*out) << endl;
        
        (*out) << "------------------------------------------" << endl << endl;
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    for(list<NetworkTreeNode*>::iterator i = childrenN.begin(); i != childrenN.end(); ++i)
    {
        neighborhoodsRecursive(out, tree, (*i));
    }
}

void NetworkTree::listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur)
{
    unsigned short type = cur->getType();

    // Storing subnets in leaves
    if(type == NetworkTreeNode::T_SUBNET)
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

void NetworkTree::toBipartiteRecursive(BipartiteGraph *bip, 
                                       NetworkTree *tree,
                                       NetworkTreeNode *cur,
                                       unsigned short depth)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    list<Router*> *routers = cur->getInferredRouters();
    
    // Puts direct neighbor subnets in a list and puts the T_NEIGHBORHOOD nodes in another one
    list<SubnetSite*> childrenS;
    list<NetworkTreeNode*> childrenN;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->getType() == NetworkTreeNode::T_SUBNET)
        {
            childrenS.push_back((*i)->getAssociatedSubnet());
        }
        else if((*i)->getType() == NetworkTreeNode::T_NEIGHBORHOOD)
        {
            childrenN.push_back((*i));
        }
    }
    
    // Goes deeper in the tree first
    for(list<NetworkTreeNode*>::iterator i = childrenN.begin(); i != childrenN.end(); ++i)
    {
        toBipartiteRecursive(bip, tree, (*i), depth + 1);
    }
    
    // Case where cur has multiple labels (i.e. it is a "load balancer" from the tree view)
    if(cur->isLoadBalancer())
    {
        for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
        {
            InetAddress ingressInterface = (*i);
            Router *ingressRouter = cur->getRouterHaving(ingressInterface); // Exists by construction
            
            // Lists children subnets which last label is ingressInterface
            list<SubnetSite*> curChildrenS;
            for(list<SubnetSite*>::iterator j = childrenS.begin(); j != childrenS.end(); ++j)
            {
                if((*j)->hasRouteLabel(ingressInterface, depth))
                {
                    curChildrenS.push_back((*j));
                    childrenS.erase(j--);
                }
            }
            list<SubnetSite*> backUpSubnets(curChildrenS);
            
            /*
             * Lists children internals which the routes to the leaves contain at least one 
             * occurrence of ingressInterface.
             */
            
            list<NetworkTreeNode*> curChildrenN;
            for(list<NetworkTreeNode*>::iterator j = childrenN.begin(); j != childrenN.end(); ++j)
            {
                if((*j)->hasPreviousLabel(ingressInterface))
                {
                    curChildrenN.push_back((*j));
                    
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
            
            if(!ingressRouter->hasBipEquivalent())
                bip->addRouter(ingressRouter);
            bipConnectWithSubnets(bip, ingressRouter, &curChildrenS);
            
            /*
             * The code now looks for routers which give access to the remaining subnets. It will 
             * connect these subnets with the selected routers, then connect the routers with an 
             * Ethernet switch to the ingress router for this label.
             */
            
            BipartiteSwitch *connectingSwitch = NULL;
            if(curChildrenS.size() > 0 && routers->size() > 1)
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    if((*j) == ingressRouter)
                        continue;
                    
                    // Router must give access to at least one listed subnet to continue
                    list<SubnetSite*>::iterator listBegin = curChildrenS.begin();
                    list<SubnetSite*>::iterator listEnd = curChildrenS.end();
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
                    bipConnectWithSubnets(bip, (*j), &curChildrenS);
                    
                    // Connects to the ingress router via an "imaginary" switch
                    if(connectingSwitch == NULL)
                    {
                        connectingSwitch = bip->createSwitch();
                        bip->createLinkSR(connectingSwitch, ingressRouter->getBipEquivalent());
                    }
                    bip->createLinkSR(connectingSwitch, (*j)->getBipEquivalent());
                }
            }
            
            // If there remains subnets, connects them with imaginary router
            if(curChildrenS.size() > 0)
            {
                list<SubnetSite*>::iterator listBegin = curChildrenS.begin();
                list<SubnetSite*>::iterator listEnd = curChildrenS.end();
                for(list<SubnetSite*>::iterator j = listBegin; j != listEnd; ++j)
                {
                    BipartiteRouter *imaginary = bip->createImaginaryRouter();
                    
                    if(connectingSwitch == NULL)
                    {
                        connectingSwitch = bip->createSwitch();
                        bip->createLinkSR(connectingSwitch, ingressRouter->getBipEquivalent());
                    }
                    
                    if(!(*j)->hasBipEquivalent())
                        bip->addSubnet((*j));
                    
                    bip->createLinkSR(connectingSwitch, imaginary);
                    bip->createLinkRS(imaginary, (*j)->getBipEquivalent());
                }
                curChildrenS.clear();
            }
            
            // Connects router with children that are internals
            if(curChildrenN.size() > 0)
            {
                bipConnectWithInternals(bip, 
                                        tree, 
                                        ingressRouter->getBipEquivalent(), 
                                        &backUpSubnets, 
                                        &curChildrenN);
            }
        }
    }
    // (simpler) Case where there is a single interface and at least one router
    else if(routers->size() > 0)
    {
        InetAddress ingressInterface = labels->front();
        Router* ingressRouter = cur->getRouterHaving(ingressInterface); // Exists by construction
        list<SubnetSite*> backUpSubnets(childrenS); // Copy of childrenS for last step

        // Connects ingress router with subnets it gives access to
        if(!ingressRouter->hasBipEquivalent())
            bip->addRouter(ingressRouter);
        bipConnectWithSubnets(bip, ingressRouter, &childrenS);
        
        // Code proceeds with other routers in the neighborhood (if any)
        BipartiteSwitch *connectingSwitch = NULL;
        if(routers->size() > 1)
        {
            for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
            {
                if((*i) == ingressRouter)
                    continue;
                
                if(!(*i)->hasBipEquivalent())
                    bip->addRouter((*i));
                bipConnectWithSubnets(bip, (*i), &childrenS);
                
                // Connects to the ingress router via an "imaginary" switch
                if(connectingSwitch == NULL)
                {
                    connectingSwitch = bip->createSwitch();
                    bip->createLinkSR(connectingSwitch, ingressRouter->getBipEquivalent());
                }
                bip->createLinkSR(connectingSwitch, (*i)->getBipEquivalent());
            }
        }
        
        // If there remains subnets, creates an imaginary router for each of them
        if(childrenS.size() > 0)
        {
            for(list<SubnetSite*>::iterator i = childrenS.begin(); i != childrenS.end(); ++i)
            {
                BipartiteRouter *imaginary = bip->createImaginaryRouter();
                
                if(connectingSwitch == NULL)
                {
                    connectingSwitch = bip->createSwitch();
                    bip->createLinkSR(connectingSwitch, ingressRouter->getBipEquivalent());
                }
                
                if(!(*i)->hasBipEquivalent())
                    bip->addSubnet((*i));
                
                bip->createLinkSR(connectingSwitch, imaginary);
                bip->createLinkRS(imaginary, (*i)->getBipEquivalent());
            }
            childrenS.clear();
        }
        
        // Connects router with children that are internals
        if(childrenN.size() > 0)
        {
            bipConnectWithInternals(bip, 
                                    tree, 
                                    ingressRouter->getBipEquivalent(), 
                                    &backUpSubnets, 
                                    &childrenN);
        }
    }
    // Case where there is no router at all
    else
    {
        // Creates an imaginary router for the whole neighborhood
        BipartiteRouter* ingressRouter = bip->createImaginaryRouter();
        cur->setImaginaryRouter(ingressRouter);
        
        // Connects all subnets to it
        for(list<SubnetSite*>::iterator i = childrenS.begin(); i != childrenS.end(); ++i)
        {
            if(!(*i)->hasBipEquivalent())
                bip->addSubnet((*i));
            
            bip->createLinkRS(ingressRouter, (*i)->getBipEquivalent());
        }
        
        // Connects imaginary router with children that are internals
        if(childrenN.size() > 0)
        {
            bipConnectWithInternals(bip, 
                                    tree, 
                                    ingressRouter, 
                                    &childrenS, 
                                    &childrenN);
        }
    }
}

void NetworkTree::bipConnectWithSubnets(BipartiteGraph *bip,
                                        Router *ingressRouter,
                                        list<SubnetSite*> *childrenS)
{
    for(list<SubnetSite*>::iterator i = childrenS->begin(); i != childrenS->end(); ++i)
    {
        if(ingressRouter->givesAccessTo((*i)))
        {
            if(!(*i)->hasBipEquivalent())
                bip->addSubnet((*i));
            
            bip->createLink(ingressRouter, (*i));
            childrenS->erase(i--);
        }
    }
}

void NetworkTree::bipConnectWithInternals(BipartiteGraph *bip,
                                          NetworkTree *tree,
                                          BipartiteRouter *ingressRouter,
                                          list<SubnetSite*> *childrenS,
                                          list<NetworkTreeNode*> *childrenN)
{
    for(list<NetworkTreeNode*>::iterator i = childrenN->begin(); i != childrenN->end(); ++i)
    {
        list<InetAddress> *labelsChild = (*i)->getLabels();
        
        // Internal is not a load balancing node
        if(labelsChild->size() == 1)
        {
            InetAddress labelChild = labelsChild->front();
            
            bipConnectWithInternal(bip,
                                   tree,
                                   ingressRouter,
                                   childrenS,
                                   (*i), 
                                   labelChild, 
                                   (*i)->getRouterHaving(labelChild));
        }
        else
        {
            // Create a link (with or without imaginary subnet) for each router containing a label
            list<InetAddress> copyLabels;
            for(list<InetAddress>::iterator j = labelsChild->begin(); j != labelsChild->end(); ++j)
                copyLabels.push_back(InetAddress((*j)));
            
            for(list<InetAddress>::iterator j = copyLabels.begin(); j != copyLabels.end(); ++j)
            {
                Router *ingressRouterChild = (*i)->getRouterHaving((*j)); // Exists by construction

                /*
                 * Removes from copyLabels the label different from ingressInterface which also 
                 * happen to be interfaces of the current ingress router.
                 */
                
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
                
                bipConnectWithInternal(bip,
                                       tree,
                                       ingressRouter,
                                       childrenS, 
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
                                         list<SubnetSite*> *childrenS,
                                         NetworkTreeNode *childN,
                                         InetAddress labelChild,
                                         Router* ingressRouterChild)
{
    bool successfullyConnected = false;
        
    /*
     * Tries to find the subnet being crossed to reach this internal and connects this 
     * subnet with the router of the internal which has the label among its interfaces.
     */
    
    for(list<SubnetSite*>::iterator i = childrenS->begin(); i != childrenS->end(); ++i)
    {
        SubnetSite *ss = (*i);
        if(ss->contains(labelChild))
        {
            if(ingressRouterChild == NULL)
            {
                BipartiteRouter *bipRouter = childN->getImaginaryRouter();
                BipartiteSubnet *bipSubnet = ss->getBipEquivalent();
                bip->createLinkRS(bipRouter, bipSubnet);
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
        
            // Creates the link with ingress if it does not exist yet
            string bipSubnetLabel = connectingSubnet->getBipEquivalent()->getLabel();
            if(!ingressRouter->isConnectedToSubnet(bipSubnetLabel))
                bip->createLinkRS(ingressRouter, connectingSubnet->getBipEquivalent());
        
            // Gets the ingress router of this child node and connects with subnet
            if(ingressRouterChild == NULL)
            {
                BipartiteRouter *bipRouter = childN->getImaginaryRouter();
                BipartiteSubnet *bipSubnet = connectingSubnet->getBipEquivalent();
                bip->createLinkRS(bipRouter, bipSubnet);
            }
            else
            {
                bip->createLink(ingressRouterChild, connectingSubnet);
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
