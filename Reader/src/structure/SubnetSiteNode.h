/*
 * SubnetSiteNode.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (October 2014) until January and adapted for TreeNET Reader in 
 * January 2015.
 */

#ifndef SUBNETSITENODE_H_
#define SUBNETSITENODE_H_

#include <iostream>
using std::ostream;

#include "../common/inet/InetAddress.h"

class SubnetSiteNode
{
public:

    // Overriden << operator, only used by debug mode (see also SubnetSite.h)
    friend ostream &operator<<(ostream &out, const SubnetSiteNode &ssn)
    {
		out << "{"
		<< *(ssn.ip.getBinaryRepresentation())
	    << "   "
	    << *(ssn.ip.getHumanReadableRepresentation())
	    << " - "
	    << (int) ssn.TTL
	    << "}";
		return out;
	}

	// Comparison method for sorting purposes
	inline static bool smaller(SubnetSiteNode *ssn1, SubnetSiteNode *ssn2) { return ssn1->ip < ssn2->ip; }

    // Constructor, destructor and private fields
	SubnetSiteNode(const InetAddress &ip, unsigned char TTL);
	virtual ~SubnetSiteNode();
	InetAddress ip;
	unsigned char TTL;
};

#endif /* SUBNETSITENODE_H_ */

