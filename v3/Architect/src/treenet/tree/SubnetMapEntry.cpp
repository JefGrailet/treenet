/*
 * SubnetMapEntry.cpp
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetMapEntry.h (see this file to learn further about the 
 * goals of such class).
 */

#include "SubnetMapEntry.h"

SubnetMapEntry::SubnetMapEntry(SubnetSite *subnet, 
                               NetworkTree *tree, 
                               NetworkTreeNode *node)
{
    this->subnet = subnet;
    this->tree = tree;
    this->node = node;
}

SubnetMapEntry::~SubnetMapEntry()
{
}

bool SubnetMapEntry::compare(SubnetMapEntry *sme1, SubnetMapEntry *sme2)
{
    return SubnetSite::compare(sme1->subnet, sme2->subnet);
}
