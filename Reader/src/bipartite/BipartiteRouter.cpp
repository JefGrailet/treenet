/*
 * BipartiteRouter.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in BipartiteRouter.h (see this file to learn further about the 
 * goals of such class).
 */

#include "BipartiteRouter.h"

BipartiteRouter::BipartiteRouter(string label)
{
    this->label = label;
    this->type = BipartiteRouter::T_IMAGINARY;
    this->associatedRouter = NULL;
}

BipartiteRouter::BipartiteRouter(string label, Router *router)
{
    this->label = label;
    this->type = BipartiteRouter::T_INFERRED;
    this->associatedRouter = router;
}

BipartiteRouter::~BipartiteRouter()
{
    // No deletion of associated router, as it should be done by tree deletion
}

bool BipartiteRouter::isConnectedToSwitch(string labelSwitch)
{
    for(list<LinkSwitchRouter*>::iterator i = linksSwitches.begin(); i != linksSwitches.end(); ++i)
    {
        if((*i)->getSwitch()->getLabel().compare(labelSwitch) == 0)
            return true;
    }
    return false;
}

void BipartiteRouter::removeConnectionToSwitch(string labelSwitch)
{
    for(list<LinkSwitchRouter*>::iterator i = linksSwitches.begin(); i != linksSwitches.end(); ++i)
    {
        if((*i)->getSwitch()->getLabel().compare(labelSwitch) == 0)
        {
            linksSwitches.erase(i--);
            return;
        }
    }
}

bool BipartiteRouter::isConnectedToSubnet(string labelSubnet)
{
    for(list<LinkRouterSubnet*>::iterator i = linksSubnets.begin(); i != linksSubnets.end(); ++i)
    {
        if((*i)->getSubnet()->getLabel().compare(labelSubnet) == 0)
            return true;
    }
    return false;
}

void BipartiteRouter::removeConnectionToSubnet(string labelSubnet)
{
    for(list<LinkRouterSubnet*>::iterator i = linksSubnets.begin(); i != linksSubnets.end(); ++i)
    {
        if((*i)->getSubnet()->getLabel().compare(labelSubnet) == 0)
        {
            linksSubnets.erase(i--);
            return;
        }
    }
}

