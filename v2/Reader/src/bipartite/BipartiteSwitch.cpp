/*
 * BipartiteSwitch.cpp
 *
 *  Created on: Mar 6, 2015
 *      Author: grailet
 *
 * Implements the class defined in BipartiteSwitch.h (see this file to learn further about the 
 * goals of such class).
 */

#include "BipartiteSwitch.h"

BipartiteSwitch::BipartiteSwitch(string label)
{
    this->label = label;
}

BipartiteSwitch::~BipartiteSwitch()
{
}

bool BipartiteSwitch::isConnectedTo(string labelRouter)
{
    for(list<LinkSwitchRouter*>::iterator i = links.begin(); i != links.end(); ++i)
    {
        if((*i)->getRouter()->getLabel().compare(labelRouter) == 0)
            return true;
    }
    return false;
}

void BipartiteSwitch::removeConnectionTo(string labelRouter)
{
    for(list<LinkSwitchRouter*>::iterator i = links.begin(); i != links.end(); ++i)
    {
        if((*i)->getRouter()->getLabel().compare(labelRouter) == 0)
        {
            links.erase(i--);
            return;
        }
    }
}
