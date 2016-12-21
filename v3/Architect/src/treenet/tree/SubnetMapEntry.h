/*
 * SubnetMapEntry.h
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * A simple class that basically amounts to a data structure gathering in a triplet a subnet, the 
 * tree it belongs to and the node corresponding to that subnet in the same tree. It allows to 
 * get a subnet from the subnet map (see Soil.h) along pointers to the structures it belongs to.
 */

#ifndef SUBNETMAPENTRY_H_
#define SUBNETMAPENTRY_H_

#include "NetworkTree.h"
#include "../structure/SubnetSite.h"

class SubnetMapEntry
{
public:

    SubnetMapEntry(SubnetSite *subnet, 
                   NetworkTree *tree, 
                   NetworkTreeNode *node);
    ~SubnetMapEntry();
    
    SubnetSite *subnet;
    NetworkTree *tree;
    NetworkTreeNode *node;
    
    static bool compare(SubnetMapEntry *sme1, SubnetMapEntry *sme2);
    
};

#endif /* SUBNETMAPENTRY_H_ */
