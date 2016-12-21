/*
 * NetworkTree.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: jefgrailet
 *
 * Implements the class defined in NetworkTree.h (see this file to learn further about the goals 
 * of such class).
 */

#include "NetworkTree.h"

NetworkTree::NetworkTree()
{
    this->root = new NetworkTreeNode();
    this->rootOffset = 0; // Default
}

NetworkTree::~NetworkTree()
{
    delete root;
}

string NetworkTree::writeSubnets()
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
    
    return output;
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
