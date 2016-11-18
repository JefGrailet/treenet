/*
 * InetAddressSet.cpp
 *
 *  Created on: Aug 4, 2008
 *      Author: root
 */



#include "InetAddressSet.h"

InetAddressNode::InetAddressNode(InetAddress *addr, InetAddressNode *nextNode):
addrPtr(addr),
nextNodePtr(nextNode)
{
	srand(time(0));
}
InetAddressNode::~InetAddressNode(){
	//do nothing
}

InetAddressSet::InetAddressSet():
size(0),
headNodePtr(0)
{

}

InetAddressSet::~InetAddressSet() {
	size=0;
	headNodePtr=0;
}

void InetAddressSet::softReleaseMemory(){
	while(size>0){
		removeElementAt(0);
	}

}
void InetAddressSet::hardReleaseMemory(){
	while(size>0){
		delete removeElementAt(0);
	}
}

bool InetAddressSet::insert(InetAddress * newAddrPtr)throw(InvalidParameterException){
	if(newAddrPtr==0){
		throw InvalidParameterException("InetAddress given to insert method of InetAddressSet is NULL");
	}
	if(contains(*newAddrPtr)){
		return false;
	}
	//create new node
	InetAddressNode * newNodePtr=new InetAddressNode(newAddrPtr,0);
	if(isEmpty()){
		headNodePtr=newNodePtr;
	}else{
		//first locate the previous node of the insertion point
		InetAddressNode *currentNodePtr=headNodePtr;
		InetAddressNode *prevNodePtr=0;
		unsigned long int newLong=newAddrPtr->getULongAddress();
		while(currentNodePtr!=0 && newLong>currentNodePtr->addrPtr->getULongAddress()){
			prevNodePtr=currentNodePtr;
			currentNodePtr=currentNodePtr->nextNodePtr;
		}

		if(prevNodePtr==0){//insert at the very first start of the list
			newNodePtr->nextNodePtr=currentNodePtr;
			headNodePtr=newNodePtr;
		}else if(currentNodePtr==0){//insert at the very end of the list
			prevNodePtr->nextNodePtr=newNodePtr;
		}else{//insert into the middle
			prevNodePtr->nextNodePtr=newNodePtr;
			newNodePtr->nextNodePtr=currentNodePtr;
		}
	}
	size++;
	return true;

}
bool InetAddressSet::contains(const InetAddress &keyAddr)const{
	return getPositionIndex(keyAddr)!=-1;
}

long InetAddressSet::getPositionIndex(const InetAddress &addr)const{
	long low=0;
	long high=size-1;
	long mid;
	while(low<=high){
		mid=(low+high)/2;
		if(addr.getULongAddress()<getElementAt(mid).getULongAddress()){
			high=mid-1;
		}else if(addr.getULongAddress()>getElementAt(mid).getULongAddress()){
			low=mid+1;
		}else{
			return mid;
		}
	}

	return -1;

}

InetAddress & InetAddressSet::getElementAt(long position)const throw(OutOfBoundException){
	if(position<0 || position>(size-1)){
		throw OutOfBoundException("The postion requested is NOT within the boundaries of the InetAddressSet collection");
	}
	InetAddressNode * currentNodePtr=headNodePtr;
	for(long i=0;i<position;i++){
		currentNodePtr=currentNodePtr->nextNodePtr;
	}

	return *(currentNodePtr->addrPtr);
}

InetAddress * InetAddressSet::removeElementAt(long position)throw(OutOfBoundException){
	if(position<0 || position>(size-1)){
		throw OutOfBoundException("The postion requested is NOT within the boundaries of the InetAddressSet collection");
	}
	InetAddressNode *currentNodePtr=headNodePtr;
	InetAddressNode *prevNodePtr=0;

	for(long i=0;i<position;i++){
		prevNodePtr=currentNodePtr;
		currentNodePtr=currentNodePtr->nextNodePtr;
	}


	if(prevNodePtr==0){//remove the very first element
		headNodePtr=headNodePtr->nextNodePtr;
	}else{
		prevNodePtr->nextNodePtr=currentNodePtr->nextNodePtr;
	}
	InetAddress *tmp=currentNodePtr->addrPtr;
	delete currentNodePtr;
	size--;
	return tmp;
}

InetAddress * InetAddressSet::removeElement(const InetAddress &key){
	long position=getPositionIndex(key);
	if(position==-1){
		return 0;
	}else{
		return removeElementAt(position);
	}
}



