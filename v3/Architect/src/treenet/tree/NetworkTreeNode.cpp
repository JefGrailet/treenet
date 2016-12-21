/*
 * NetworkTreeNode.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: jefgrailet
 *
 * Implements the class defined in NetworkTreeNode.h (see this file to learn further about the 
 * goals of such class).
 */

#include "NetworkTreeNode.h"

NetworkTreeNode::NetworkTreeNode()
{
    this->labels.push_back(InetAddress(0));
    this->type = NetworkTreeNode::T_ROOT;
    this->associatedSubnet = NULL;
    this->parent = NULL;
}

NetworkTreeNode::NetworkTreeNode(InetAddress label)
{
    this->labels.push_back(label);
    this->type = NetworkTreeNode::T_NEIGHBORHOOD;
    this->associatedSubnet = NULL;
    this->parent = NULL;
}

NetworkTreeNode::NetworkTreeNode(SubnetSite *subnet) throw (InvalidSubnetException)
{
    // Cannot create node with a subnet that is neither ACCURATE, neither SHADOW nor ODD
    if(subnet->getStatus() != SubnetSite::ACCURATE_SUBNET &&
       subnet->getStatus() != SubnetSite::SHADOW_SUBNET &&
       subnet->getStatus() != SubnetSite::ODD_SUBNET)
    {
        throw new InvalidSubnetException(subnet->getInferredNetworkAddressString());
    }

    if (subnet->getStatus() == SubnetSite::SHADOW_SUBNET)
        this->labels.push_back(subnet->getPivot());
    else
        this->labels.push_back(subnet->getContrapivot());
    this->type = NetworkTreeNode::T_SUBNET;
    this->associatedSubnet = subnet;
    this->parent = NULL;
}

NetworkTreeNode::~NetworkTreeNode()
{
    if(type == NetworkTreeNode::T_SUBNET)
    {
        delete this->associatedSubnet;
        this->associatedSubnet = NULL;
    }
    else
    {
        for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
        {
            delete (*i);
        }
        children.clear();
        
        for(list<Router*>::iterator i = inferredRouters.begin(); i != inferredRouters.end(); ++i)
        {
            delete (*i);
        }
        inferredRouters.clear();
    }
}

bool NetworkTreeNode::isRoot()
{
    if(type == NetworkTreeNode::T_ROOT)
        return true;
    return false;
}

bool NetworkTreeNode::isLeaf()
{
    if(type == NetworkTreeNode::T_SUBNET)
        return true;
    return false;
}

bool NetworkTreeNode::isInternal()
{
    if(type == NetworkTreeNode::T_NEIGHBORHOOD || type == NetworkTreeNode::T_HEDERA)
        return true;
    return false;
}

bool NetworkTreeNode::isHedera()
{
    if(type == NetworkTreeNode::T_HEDERA)
        return true;
    return false;
}

bool NetworkTreeNode::hasLabel(InetAddress label)
{
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if((*i) == label)
            return true;
    }
    return false;
}

bool NetworkTreeNode::hasPreviousLabel(InetAddress label)
{
    for(list<InetAddress>::iterator i = previousLabels.begin(); i != previousLabels.end(); ++i)
    {
        if((*i) == label)
            return true;
    }
    return false;
}

void NetworkTreeNode::addLabel(InetAddress label)
{
    InetAddress previousHead = labels.front();

    labels.push_back(label);
    if(labels.size() > 1 && this->type == NetworkTreeNode::T_NEIGHBORHOOD)
        this->type = NetworkTreeNode::T_HEDERA;
    labels.sort(InetAddress::smaller);
    
    /*
     * If the head of the list changed, one must re-sort the list of children of the parent node 
     * (if any) to keep a coherent order.
     */
    
    if(previousHead != labels.front())
    {
        if(this->parent != NULL)
            this->parent->sortChildren();
    }
}

void NetworkTreeNode::addPreviousLabel(InetAddress label)
{
    for(list<InetAddress>::iterator i = previousLabels.begin(); i != previousLabels.end(); ++i)
    {
        if((*i) == label)
            return;
    }
    previousLabels.push_back(label);
    previousLabels.sort(InetAddress::smaller);
}

bool NetworkTreeNode::compare(NetworkTreeNode *n1, NetworkTreeNode *n2)
{
    list<InetAddress> *labels1 = n1->getLabels();
    list<InetAddress> *labels2 = n2->getLabels();
    
    bool result = false;
    if (labels1->front() < labels2->front())
        result = true;
    return result;
}

void NetworkTreeNode::addChild(NetworkTreeNode *child)
{
    child->setParent(this);
    children.push_back(child);
    children.sort(NetworkTreeNode::compare);
}

void NetworkTreeNode::merge(NetworkTreeNode *mergee)
{
    // Gets the children from mergee and updates their parent
    list<NetworkTreeNode*> *mChildren = mergee->getChildren();
    for(list<NetworkTreeNode*>::iterator i = mChildren->begin(); i != mChildren->end(); ++i)
        (*i)->setParent(this);
    
    // Merges and sorts
    children.merge(*(mChildren));
    children.sort(NetworkTreeNode::compare);
}

NetworkTreeNode *NetworkTreeNode::getChild(InetAddress label)
{
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
        if((*i)->hasLabel(label))
        {
            return (*i);
        }
    }
    return NULL;
}

bool NetworkTreeNode::hasOnlyLeavesAsChildren()
{
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
        if(!(*i)->isLeaf())
        {
            return false;
        }
    }
    return true;
}

unsigned short NetworkTreeNode::getLinkage()
{
    if(this->isLeaf() || this->hasOnlyLeavesAsChildren())
        return 0;
    
    unsigned short missingLinks = 0;
    list<NetworkTreeNode*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
        if((*i)->isLeaf())
        {
            childrenL.push_back((*i));
        }
        else if((*i)->isInternal())
        {
            childrenI.push_back((*i));
        }
    }
    
    if(childrenL.size() == 0)
    {
        if(childrenI.size() > 2)
            return 2;
        else
            return 1;
    }
    
    for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
    {
        list<InetAddress> *labels = (*i)->getLabels();
        
        bool onTheWay = false;
        for(list<NetworkTreeNode*>::iterator j = childrenL.begin(); j != childrenL.end(); ++j)
        {
            SubnetSite *ss = (*j)->getAssociatedSubnet();
            
            for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
            {
                // "On the way"
                if(ss->contains((*k)))
                    onTheWay = true;
            }
        }
        
        if(!onTheWay)
            missingLinks++;
    }
    
    if(missingLinks > 1)
        return 2;
    else if(missingLinks > 0)
        return 1;
    return 0;
}

list<InetAddress> NetworkTreeNode::listInterfaces()
{
    list<InetAddress> interfacesList;
    
    // Listing labels of this node
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if((*i) != InetAddress("0.0.0.0"))
            interfacesList.push_back((*i));
    }
    
    // Listing children (only subnets)
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
        if((*i)->isLeaf())
        {
            SubnetSite *ss = (*i)->getAssociatedSubnet();
            unsigned short status = ss->getStatus();
            unsigned char shortestTTL = ss->getShortestTTL();
            
            if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
            {
                list<SubnetSiteNode*> *listSsn = ss->getSubnetIPList();
                
                for(list<SubnetSiteNode*>::iterator j = listSsn->begin(); j != listSsn->end(); ++j)
                {
                    if((*j)->TTL == shortestTTL)
                    {
                        interfacesList.push_back(((*j)->ip));
                    }
                }
            }
        }
    }
    
    // Sorts and removes duplicata (rare but possible in some cases)
    interfacesList.sort(InetAddress::smaller);
    InetAddress previous("0.0.0.0");
    for(list<InetAddress>::iterator i = interfacesList.begin(); i != interfacesList.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == previous)
        {
            interfacesList.erase(i--);
        }
        else
        {
            previous = cur;
        }
    }
    
    return interfacesList;
}

Router* NetworkTreeNode::getRouterHaving(InetAddress interface)
{
    if(inferredRouters.size() == 0)
        return NULL;
    
    for(list<Router*>::iterator i = inferredRouters.begin(); i != inferredRouters.end(); ++i)
    {
        if((*i)->hasInterface(interface))
            return (*i);
    }
    
    return NULL;
}
