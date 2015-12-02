/*
 * LinkSwitchRouter.cpp
 *
 *  Created on: Mar 6, 2015
 *      Author: grailet
 *
 * Implements the class defined in LinkSwitchRouter.h (see this file to learn further about the 
 * goals of such class).
 */

#include "LinkSwitchRouter.h"

LinkSwitchRouter::LinkSwitchRouter(BipartiteSwitch *eSwitch, BipartiteRouter *router)
{
    this->eSwitch = eSwitch;
    this->router = router;
}

LinkSwitchRouter::~LinkSwitchRouter()
{
    // No deletion at all, as it should be carried out by tree/graph deletion
}

string LinkSwitchRouter::toString()
{
    string switchStr = eSwitch->getLabel();
    string routerStr = router->getLabel();
    return switchStr + " - " + routerStr;
}
