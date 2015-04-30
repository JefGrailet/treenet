/*
 * NetworkTreeNode.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
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
    if(subnet->getRefinementStatus() != SubnetSite::ACCURATE_SUBNET &&
       subnet->getRefinementStatus() != SubnetSite::SHADOW_SUBNET &&
       subnet->getRefinementStatus() != SubnetSite::ODD_SUBNET)
    {
        throw new InvalidSubnetException(subnet->getInferredNetworkAddressString());
    }

    if (subnet->getRefinementStatus() == SubnetSite::SHADOW_SUBNET)
        this->labels.push_back(subnet->getRefinementPivot());
    else
        this->labels.push_back(subnet->getRefinementContrapivot());
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
	}
}

bool NetworkTreeNode::isLoadBalancer()
{
    if(labels.size() > 1)
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
    labels.push_back(label);
    labels.sort(InetAddress::smaller);
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

bool NetworkTreeNode::hasLeavesAsChildren()
{
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
	    if((*i)->getType() == NetworkTreeNode::T_SUBNET)
	    {
	        return true;
	    }
    }
    return false;
}

unsigned short NetworkTreeNode::getLinkage()
{
    if(this->type == NetworkTreeNode::T_SUBNET || this->hasLeavesAsChildren())
        return 0;
    
    unsigned short missingLinks = 0;
    list<NetworkTreeNode*> childrenS;
    list<NetworkTreeNode*> childrenN;
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
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
    
    if(childrenS.size() == 0)
    {
        if(childrenN.size() > 2)
            return 2;
        else
            return 1;
    }
    
    for(list<NetworkTreeNode*>::iterator i = childrenN.begin(); i != childrenN.end(); ++i)
    {
        list<InetAddress> *labels = (*i)->getLabels();
        
        bool onTheWay = false;
        for(list<NetworkTreeNode*>::iterator j = childrenS.begin(); j != childrenS.end(); ++j)
        {
            SubnetSite *ss = (*j)->getAssociatedSubnet();
            
            for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
            {
                // "On the way"
                if(ss->containsAddress((*k)))
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

list<InetAddress*> NetworkTreeNode::listInterfaces()
{
    list<InetAddress*> interfacesList;
    
    // Listing labels of this node
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        interfacesList.push_back(&(*i));
    }
    
    // Listing childrens (only subnets)
    for(list<NetworkTreeNode*>::iterator i = children.begin(); i != children.end(); ++i)
    {
        if((*i)->getType() == NetworkTreeNode::T_SUBNET)
        {
            SubnetSite *ss = (*i)->getAssociatedSubnet();
            unsigned short status = ss->getRefinementStatus();
            unsigned char shortestTTL = ss->getRefinementShortestTTL();
            
            if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
            {
                list<SubnetSiteNode*> *listSsn = ss->getSubnetIPList();
                
                for(list<SubnetSiteNode*>::iterator j = listSsn->begin(); j != listSsn->end(); ++j)
                {
                    if((*j)->TTL == shortestTTL)
                    {
                        interfacesList.push_back(&((*j)->ip));
                    }
                }
            }
        }
    }
    
    return interfacesList;
}

void NetworkTreeNode::copyRouterInfoIntoRoute()
{
    if(this->type != NetworkTreeNode::T_SUBNET)
    {
        return;
    }
    
    NetworkTreeNode *parent = this->getParent();
    InetAddress *route = this->getAssociatedSubnet()->getRefinementRoute();
    unsigned short index = this->getAssociatedSubnet()->getRefinementRouteSize() - 1;
    while(parent->getType() != NetworkTreeNode::T_ROOT)
    {
        list<InetAddress> *labels = parent->getLabels();
        for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
        {
            if((*i) == route[index])
            {
                route[index].setProbeToken((*i).getProbeToken());
                route[index].setIPIdentifier((*i).getIPIdentifier());
                route[index].setStoredHostName((*i).getStoredHostName());
                break;
            }
        }
    
        parent = parent->getParent();
        index--;
    }
}

list<Router> NetworkTreeNode::inferRouters()
{
    list<InetAddress*> interfaces = this->listInterfaces();
    
    // Remove duplicata (possible because ingress interface of neighborhood can be a contra-pivot)
    InetAddress previous(0);
    for(list<InetAddress*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        InetAddress *current = (*i);
        if(current == NULL)
            continue;
        
        if(current != NULL && (*current) == previous)
            interfaces.erase(i--);
        
        previous = (*current);
    }

    // First compute the smallest gap between IP IDs.
    unsigned short smallestGap = 65335;
    bool noHostNames = true;
    for(list<InetAddress*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        for(list<InetAddress*>::iterator j = i; j != interfaces.end(); ++j)
        {
            if((*i) == (*j))
                continue;
             
            if(!(*i)->getStoredHostName().empty() && !(*j)->getStoredHostName().empty())
                noHostNames = false;
            
            if((*i)->hasIPIdentifier() && (*j)->hasIPIdentifier())
            {
                unsigned short thisGap = 0;
                unsigned short IPId1 = (*i)->getIPIdentifier();
                unsigned short IPId2 = (*j)->getIPIdentifier();
                if(IPId1 > IPId2)
                    thisGap = IPId1 - IPId2;
                else
                    thisGap = IPId2 - IPId1;
                
                if(thisGap > (65535 - MAX_IP_ID_DIFFERENCE))
                {
                    if(IPId1 > 60000)
                        thisGap = (65535 - IPId1) + IPId2;
                    else
                        thisGap = (65535 - IPId2) + IPId1;
                }
                
                if(thisGap < smallestGap)
                    smallestGap = thisGap;
            }
        }
    }
    
    // If not enough host names, above a threshold (for the gap), infers a router per interface.
    if(noHostNames && smallestGap > NO_ASSOCIATION_THRESHOLD)
    {
        list<Router> result;
        for(list<InetAddress*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        {
            Router curRouter;
            curRouter.addInterface(InetAddress(*(*i)));
            result.push_back(curRouter);
        }
        
        /*
         * Some post-processing: removes the routers consisting of a single interface which happens 
         * to be a candidate contra-pivot of an ODD subnet.
         *
         * N.B.: checking the interface appears in the subnet responsive IPs list is enough, as the 
         * pivots were not listed at all in the potential interfaces.
         */
         
        for(list<Router>::iterator i = result.begin(); i != result.end(); ++i)
        {
            if((*i).getNbInterfaces() == 1)
            {
                InetAddress singleInterface = (*i).getInterfacesList()->front();
                for(list<NetworkTreeNode*>::iterator j = children.begin(); j != children.end(); ++j)
                {
                    if((*j)->getType() == NetworkTreeNode::T_SUBNET)
                    {
                        SubnetSite *ss = (*j)->getAssociatedSubnet();
                        
                        if(ss->getRefinementStatus() == SubnetSite::ODD_SUBNET && 
                           ss->hasLiveInterface(singleInterface))
                        {
                            bool isALabel = false;
                            for(list<InetAddress>::iterator k = labels.begin(); k != labels.end(); ++k)
                            {
                                if((*k) == singleInterface)
                                {
                                    isALabel = true;
                                    break;
                                }
                            }
                            
                            if(!isALabel)
                            {
                                result.erase(i--);
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        return result;
    }
    
    // Computes the delta between two interfaces to associates them.
    unsigned short delta = MAX_IP_ID_DIFFERENCE;
    if(smallestGap > delta)
    {
        delta = smallestGap + 5 * (smallestGap / MAX_IP_ID_DIFFERENCE);
    }
    
    // The maximum difference between two probes token is the size of the set.
    unsigned int deltaTokens = (unsigned int) interfaces.size();
    
    /*
     * Before association, we re-order the interfaces such that the interfaces with host names are 
     * treated first.
     */
    
    list<InetAddress*> reOrdered;
    for(list<InetAddress*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        InetAddress *cur = (*i);
        if(!cur->getStoredHostName().empty())
        {
            reOrdered.push_back(cur);
            interfaces.erase(i--);
        }
    }
    
    for(list<InetAddress*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        reOrdered.push_back((*i));
        interfaces.erase(i--);
    }
    
    // Starts association...
    list<Router> result;
    while(reOrdered.size() > 0)
    {
        InetAddress *cur = reOrdered.front();
        string curHostName = cur->getStoredHostName();
        bool testCur = cur->hasIPIdentifier();
        reOrdered.pop_front();
        
        if(!testCur && curHostName.empty())
            continue;
        
        Router curRouter;
        curRouter.addInterface(InetAddress(*cur));
        unsigned int curProbeToken = cur->getProbeToken();
        unsigned short curIPId = cur->getIPIdentifier();

        for(list<InetAddress*>::iterator i = reOrdered.begin(); i != reOrdered.end(); ++i)
        {
            string hostName = (*i)->getStoredHostName();
            bool testI = (*i)->hasIPIdentifier();
            
            // Association by host names
            if(!curHostName.empty() && !hostName.empty())
            {
                list<string> chnChunks;
                list<string> hnChunks;
                string element;
                
                stringstream chn(curHostName);
                while (std::getline(chn, element, '.'))
                {
                    chnChunks.push_front(element);
                }
                
                stringstream hn(hostName);
                while (std::getline(hn, element, '.'))
                {
                    hnChunks.push_front(element);
                }
                
                if(chnChunks.size() == hnChunks.size())
                {
                    unsigned short size = (unsigned short) chnChunks.size();
                    unsigned short similarities = 0;
                    
                    list<string>::iterator itBis = hnChunks.begin();
                    for(list<string>::iterator it = chnChunks.begin(); it != chnChunks.end(); ++it)
                    {
                        if((*it).compare((*itBis)) == 0)
                        {
                            similarities++;
                            itBis++;
                        }
                        else
                            break;
                    }
                    
                    if(similarities >= (size - 1))
                    {
                        curRouter.addInterface(InetAddress(*(*i)));
                        reOrdered.erase(i--);
                    }
                }
            }
            // Association by IP IDs (when one of both IPs does not have a host name)
            else if((curHostName.empty() || hostName.empty()) && testI && testCur)
            {
                unsigned int probeToken = (*i)->getProbeToken();
                unsigned short IPId = (*i)->getIPIdentifier();
                
                unsigned int diffProbeToken = 0;
                unsigned short diffIPId = 0;
                
                if(curProbeToken > probeToken)
                    diffProbeToken = curProbeToken - probeToken;
                else
                    diffProbeToken = probeToken - curProbeToken;
                
                if(curIPId > IPId)
                    diffIPId = curIPId - IPId;
                else
                    diffIPId = IPId - curIPId;
                
                if(diffIPId > (65535 - MAX_IP_ID_DIFFERENCE))
                {
                    if(curIPId > 60000)
                        diffIPId = (65535 - curIPId) + IPId;
                    else
                        diffIPId = (65535 - IPId) + curIPId;
                }
                
                if(diffProbeToken <= deltaTokens && diffIPId <= delta)
                {
                    curRouter.addInterface(InetAddress(*(*i)));
                    reOrdered.erase(i--);
                }
            }
        }
        result.push_back(curRouter);
    }
    
    /*
     * Some post-processing: removes the routers consisting of a single interface which happens 
     * to be a candidate contra-pivot of an ODD subnet, except if it is among the labels of this 
     * node.
     *
     * N.B.: checking the interface appears in the subnet responsive IPs list is enough, as the 
     * pivots were not listed at all in the potential interfaces.
     */
     
    for(list<Router>::iterator i = result.begin(); i != result.end(); ++i)
    {
        if((*i).getNbInterfaces() == 1)
        {
            InetAddress singleInterface = (*i).getInterfacesList()->front();
            for(list<NetworkTreeNode*>::iterator j = children.begin(); j != children.end(); ++j)
            {
                if((*j)->getType() == NetworkTreeNode::T_SUBNET)
                {
                    SubnetSite *ss = (*j)->getAssociatedSubnet();
                    
                    if(ss->getRefinementStatus() == SubnetSite::ODD_SUBNET && 
                       ss->hasLiveInterface(singleInterface))
                    {
                        bool isALabel = false;
                        for(list<InetAddress>::iterator k = labels.begin(); k != labels.end(); ++k)
                        {
                            if((*k) == singleInterface)
                            {
                                isALabel = true;
                                break;
                            }
                        }
                        
                        if(!isALabel)
                        {
                            result.erase(i--);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

