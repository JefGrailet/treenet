/*
 * SubnetSiteNode.cpp
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (october 2014) to improve coding style and study the code.
 */

#include "SubnetSiteNode.h"

string SubnetSiteNode::subnetSiteAliasDiscoveryMethodToString(enum SubnetSiteNode::AliasDiscoveryMethod method)
{
    switch(method)
    {
        case SubnetSiteNode::UNKNOWN_ALIAS_SX:
            return "UNKNOWN_ALIAS";
            break;
        case SubnetSiteNode::PALMTREE_BASED_ALIAS_SX:
            return "PALMTREE_BASED_ALIAS";
            break;
        case SubnetSiteNode::MATE3031_BASED_ALIAS_SX:
            return "MATE3031_BASED_ALIAS";
            break;
        case SubnetSiteNode::SOURCE_BASED_ALIAS_SX:
            return "SOURCE_BASED_ALIAS";
            break;
        default:
            return "ERROR_ALIAS_SX";
    }
}

string SubnetSiteNode::subnetSiteNodeStatusToString(enum SubnetSiteNode::SubnetSiteNodeStatus nodeStatus)
{
    switch(nodeStatus)
    {
        case SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES:
            return "INSIDE_SUBNET_BOUNDARIES";
            break;
        case SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY:
            return "BOUNDARY_ADDRESS_INCOMPATIBILITY";
            break;
        case SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH:
            return "SUBNET_OVERGROWTH";
            break;
        default:
            return "ERROR_IN_NODE_STATUS";
    }

}

SubnetSiteNode::SubnetSiteNode(const InetAddress &i, 
                               unsigned char p, 
                               unsigned char T, 
                               enum SubnetSiteNode::AliasDiscoveryMethod as, 
                               bool atFilling):
ip(i),
prefix(p),
TTL(T),
aliasStatus(as),
nodeStatus(SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES), 
addedAtFilling(atFilling)
{
}

SubnetSiteNode::~SubnetSiteNode() {}
