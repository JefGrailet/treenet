/*
 * InvalidSubnetException.h
 *
 *  Created on: Nov 16, 2014
 *      Author: grailet
 *
 * An exception that can be thrown by NetworkTreeNode(SubnetSite *subnet) when the given subnet 
 * is neither accurate, nor odd (i.e. the subnet is most probably imprecise).
 */

#ifndef INVALIDSUBNETEXCEPTION_H_
#define INVALIDSUBNETEXCEPTION_H_

#include "../common/exception/NTmapException.h"
#include "../common/inet/InetAddress.h"
#include <string>
using std::string;

class InvalidSubnetException: public NTmapException
{
public:

    InvalidSubnetException(const string &subnetString,
                           const string &msg = "This subnet is not suitable for a network tree");
    virtual ~InvalidSubnetException() throw();
    string getSubnetString() { return subnetString; }
    
private:
    string subnetString;
};

#endif /* INVALIDSUBNETEXCEPTION_H_ */
