/*
 * InetAddressSet.h
 *
 *  Created on: Aug 4, 2008
 *      Author: root
 */

#ifndef INETADDRESSSET_H_
#define INETADDRESSSET_H_


#include <iostream>
using std::ostream;
using std::cout;
using std::endl;
#include <cstdlib>
#include <ctime>


#include "../exception/InvalidParameterException.h"
#include "../exception/OutOfBoundException.h"
#include "../exception/EmptyCollectionException.h"
#include "InetAddress.h"

/**
 * This is just a structure class to represent node of a linked list
 */
class InetAddressNode{
public:
	InetAddressNode(InetAddress *addrPtr=0, InetAddressNode *nextNodePtr=0);
	virtual ~InetAddressNode();
	InetAddress *addrPtr;
	InetAddressNode *nextNodePtr;
};

/**
 * InetAddressSet is just a set that is always kept sorted in increasing order by IP addresses.
 * The set is kept ordered in order to reduce the complexity of searching.
 */
class InetAddressSet {
	friend ostream & operator<<(ostream &out, const InetAddressSet &set){
		out<<"Size:"<<set.getSize();
		InetAddressNode * currentNodePtr=set.headNodePtr;
		while(currentNodePtr!=0){
			out<<" "<<*(currentNodePtr->addrPtr->getHumanReadableRepresentation())
			<<"("<<currentNodePtr->addrPtr->getULongAddress()<<")";
			currentNodePtr=currentNodePtr->nextNodePtr;
		}
		return out;
	}
public:
	InetAddressSet();
	virtual ~InetAddressSet();
	/**
	 * InetAddressset insert() method accepts the memory address of an InetAddress object. To
	 * keep the structure as a sorted set (underlied by a linked list), it dynamically creates
	 * an InetAddressNode object and puts the address of the object into that node. Hence there
	 * are two memory blocks for an InetAddress, the InetAddress object itself and the InetAddressNode
	 * object.
	 *
	 * softReleaseMemory: releases the memory allocated for the InetAddressNode objects and detaches
	 * all InetAddress objects from the set. (i.e. the set becomes empty) but does not touch to the
	 * memory locations of the InetAddress objects. For example this method can be used after copying
	 * all InetAddress' from a temporary set to another set to release the memory of the temporary set.
	 * With this method even though the linked list under the set structure has been released, the actual
	 * InetAddress' are alive.
	 *
	 * hardReleaseMemory:releases the memory of all InetAddressNode objects as well as InetAddress objects
	 * do after this method is called the set becomes empty, and all InetAddress objects are dead.
	 *
	 */
	void softReleaseMemory();
	void hardReleaseMemory();
	//query methods
	bool isEmpty()const{return size==0;}
	long getSize()const{return size;}
	bool contains(const InetAddress &addr)const;
	/**
	 * Returns the position index if the element is in this set
	 * otherwise returns -1
	 */
	long getPositionIndex(const InetAddress &addr)const;
	InetAddress & getElementAt(long position)const throw(OutOfBoundException);
	//set methods
	/**
	 * Adds the @param newAddrPtr to the set and returns true if the set does not
	 * already contains the lement otherwise it does not insert and returns false.
	 * If the @param newAddrPtr is NULL throws InvalidParameterException.
	 */
	bool insert(InetAddress * newAddrPtr)throw(InvalidParameterException);
	/**
	 * Removes the element from the set. The ownership of the removed InetAddress
	 * is returned to the caller as well.
	 */
	InetAddress * removeElementAt(long position)throw(OutOfBoundException);
	InetAddress * removeElement(const InetAddress &key);
	InetAddress * removeRandomElement()throw(OutOfBoundException){
		return (size>0 ? removeElementAt(rand()%size) : throw OutOfBoundException("The set is empty"));
	}



private:
	long size;
	InetAddressNode *headNodePtr;
};

#endif /* INETADDRESSSET_H_ */
