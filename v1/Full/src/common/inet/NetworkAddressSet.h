/*
 * NetworkAddressList.h
 *
 *  Created on: Jul 28, 2008
 *      Author: root
 */

#ifndef NETWORKADDRESSSET_H_
#define NETWORKADDRESSSET_H_

#include <iostream>
using std::ostream;
using std::cout;
using std::endl;


#include "../exception/InvalidParameterException.h"
#include "../exception/OutOfBoundException.h"
#include "../exception/EmptyCollectionException.h"
#include "InetAddress.h"


class NetworkAddress;//forward declaration
/**
 * This is just a structure class to represent node of a linked list
 */
class NetworkAddressNode{
public:
	NetworkAddressNode(NetworkAddress *subnetPtr=0, NetworkAddressNode * nextNodePtr=0);
	virtual ~NetworkAddressNode();
	NetworkAddress *subnetPtr;
	NetworkAddressNode *nextNodePtr;
};

/**
 * NetworkAddressSet is a set containing NetworkAddress'  always sorted in
 * decreasing prefix length. Methods aggregate and clear releases the memory
 * NetworkAddress' hence the inserted NetworkAddress' must be dynamically
 * allocated addresses. If you have mixed heap and stack memory objects, call
 * clone to get a pure heap allocated set, and run the method over this new set.
 *
 *
 * This class must be used with extreme care because the default order of the elements
 * is DESCENDING_PREFIX_LENGTH_ORDER  contains() and subsumes() methods run linear in this
 * order so i is slow. On the other hand clear() and aggregate() methods can only run
 * correctly if the order is DESCENDING_PREFIX_LENGTH_ORDER. Howerver, if we set the
 * order to ASCENDING_PREFIX_ORDER then contains() and subsumes() methods run binary
 * which is very fast. Yet, the binary versions of contains() and subsumes() methods
 * can only run correctly if the set is aggregated.
 *
 * As a rule of thumb:
 * 1-populate the network address set without changing its order
 * 2-apply clear method if required
 * 3-If you want to have fast search
 * 			-aggregate() the set
 * 			-sort(ASCENDING_PREFIX_ORDER)
 * But after sorting by ASCENDING_PREFIX_ORDER if you insert a new item
 * you must again aggregate in order to make contains() and subsumes() work correctly.
 */

enum NetworkAddressSetOrder{DESCENDING_PREFIX_LENGTH_ORDER, ASCENDING_PREFIX_LENGTH_ORDER, ASCENDING_PREFIX_ORDER, NO_ORDER};

class NetworkAddressSet {
	friend ostream & operator<<(ostream &out, const NetworkAddressSet &set);
public:
	NetworkAddressSet();
	virtual ~NetworkAddressSet();
	//query methods
	bool isEmpty()const{return this->size==0;}
	NetworkAddressSet * clone();
	NetworkAddressSet * extract(int low, int subSize=-1)throw(OutOfBoundException);
	//set methods
	/**
	 * Inserts the @param subnet into the set, if the set already
	 * contains an element with the same prefix mask and prefixLength
	 * it does not add it and returns false. If the subnet is NULL
	 * throws invalid parameter exception
	 */
	bool insert(NetworkAddress * subnet)throw(InvalidParameterException);

	//get methods
	int getSize()const{return this->size;}

	NetworkAddress * getNetworkAddressAt(int position)throw (OutOfBoundException);

	InetAddress getRandomAddress()throw(EmptyCollectionException);
	NetworkAddressSetOrder getOrder()const {return order;}
	void sort(NetworkAddressSetOrder newOrder);

	/**
	 * Returns the frequency of each /x subnet in the set where 0<= x <=NetworkAddress::MAX_PREFIX_LENGTH
	 * as an array of size NetworkAddress::MAX_PREFIX_LENGTH + 1.
	 * The array is created dynamically and the ownership is returned to the caller as well.
	 */
	int * getPrefixFrequencyDistribution();




	/**
	 * Removes the element at @param position from the set. The ownership
	 * of the pointer is returned as well. Hence the caller must explicitly
	 * release the memory of the object.
	 */
	NetworkAddress * removeNetworkAddressAt(int position)throw(OutOfBoundException);
	/**
	 * Removes NetworkAddresses based on pointer (physical memory address of @toRemove)
	 * */
	NetworkAddress * removeNetworkAddress(NetworkAddress *toRemovePtr);
	/**
	 * Removes NetworkAddress' based on equality of prefixes and prefixLengths
	 */
	NetworkAddress * removeNetworkAddress(NetworkAddress &toRemove);
	/**
	 * If there is an element with the exact subnetPrefix and subnetPrefixLength
	 * in the set it returns the position where it is located. Otherwise returns
	 * -1. The contains method is based on exact match.
	 */
	bool contains(const NetworkAddress &subnet);
	bool subsumes(const NetworkAddress &subnet)const;
	bool subsumes(const InetAddress &ip)const;
	void aggregate();
	/**
	 * Clears all the network addresses given in the @param customerSet.
	 * The assumption is this NetworkAddressSet is the set of NetworkAddress'
	 * belonging to a major ISP who sells some NetworkAddress' to
	 * customers given at @paramcustomerSet. The algorithm takes each NetworkAddress
	 * in the customer set and scans through this set (major ISP) if the major
	 * set has a NetworkAddress which contains the customer NetworkAddress; it
	 * appropriately splits the NetworkAddress into parts, removes the customer
	 * NetworkAddress and reaggregates the major NetworkAddress.
	 */
	void clear(NetworkAddressSet &customerSet, bool verbose);
	void hardReleaseMemory();
	void softReleaseMemory();
	/**
	 * Returns position of the node if it is found else -1 is returned.
	 * However, make sure the order is ASCENDING_PREFIX_ORDER if you
	 * want to use binary (faster) search. If you run binary search and
	 * the order is not correct it will recognize the situation and
	 * do it via linear search.
	 */
	int containsBinary(const NetworkAddress &subnet);
	int containsLinear(const NetworkAddress &subnet);
	/**
	 * The following subsumes methods are meaningful only if the set has been aggregated.
	 */
	int subsumesBinary(const NetworkAddress &subnet)const;
	int subsumesLinear(const NetworkAddress &subnet)const;
	int subsumesBinary(const InetAddress &ip)const;
	int subsumesLinear(const InetAddress &ip)const;
private:
	bool swapNodes(NetworkAddressNode *n1, NetworkAddressNode *n2);


	NetworkAddressNode * getNodeAt(int position)const throw (OutOfBoundException);
	NetworkAddressNode * removeNodeAt(int position)throw (OutOfBoundException);


	int size;
	NetworkAddressNode *headNodePtr;
	NetworkAddressSetOrder order;
};

#endif /* NETWORKADDRESSLIST_H_ */
