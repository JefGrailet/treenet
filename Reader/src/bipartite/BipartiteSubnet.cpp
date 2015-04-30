/*
 * BipartiteSubnet.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in BipartiteSubnet.h (see this file to learn further about the 
 * goals of such class).
 */

#include "BipartiteSubnet.h"

BipartiteSubnet::BipartiteSubnet(string label)
{
    this->label = label;
    this->type = BipartiteSubnet::T_IMAGINARY;
    this->associatedSubnet = NULL;
}

BipartiteSubnet::BipartiteSubnet(string label, SubnetSite *subnet)
{
    this->label = label;
    this->type = BipartiteSubnet::T_INFERRED;
    this->associatedSubnet = subnet;
}

BipartiteSubnet::~BipartiteSubnet()
{
    // No deletion of associated subnet, as it should be done by tree deletion
}

bool BipartiteSubnet::isConnectedTo(string labelRouter)
{
    for(list<LinkRouterSubnet*>::iterator i = links.begin(); i != links.end(); ++i)
    {
        if((*i)->getRouter()->getLabel().compare(labelRouter) == 0)
            return true;
    }
    return false;
}

void BipartiteSubnet::removeConnectionTo(string labelRouter)
{
    for(list<LinkRouterSubnet*>::iterator i = links.begin(); i != links.end(); ++i)
    {
        if((*i)->getRouter()->getLabel().compare(labelRouter) == 0)
        {
            links.erase(i--);
            return;
        }
    }
}

