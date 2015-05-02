/*
 * LinkRouterSubnet.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in LinkRouterSubnet.h (see this file to learn further about the 
 * goals of such class).
 */

#include "LinkRouterSubnet.h"

LinkRouterSubnet::LinkRouterSubnet(BipartiteRouter *router, BipartiteSubnet *subnet)
{
    this->router = router;
    this->subnet = subnet;
}

LinkRouterSubnet::~LinkRouterSubnet()
{
    // No deletion at all, as it should be carried out by tree/graph deletion
}

bool LinkRouterSubnet::isRealLink()
{
    unsigned short tr = router->getType();
    unsigned short ts = subnet->getType();
    if(tr == BipartiteRouter::T_INFERRED && ts == BipartiteSubnet::T_INFERRED)
        return true;
    return false;
}

string LinkRouterSubnet::toString()
{
    string routerStr = router->getLabel();
    string subnetStr = subnet->getLabel();
    return routerStr + " - " + subnetStr;
}
