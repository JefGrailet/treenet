/*
 * NetworkAddressList.cpp
 *
 *  Created on: Jul 28, 2008
 *      Author: root
 */
#include <cstdlib>
#include <iostream>
using std::cout;
using std::endl;

#include "NetworkAddressSet.h"
#include "NetworkAddress.h"

ostream & operator<<(ostream &out, const NetworkAddressSet &set){
		NetworkAddressNode * currentNodePtr=set.headNodePtr;
		out<<"Size:"<<set.size<<endl;
		while(currentNodePtr!=0){
			out<<"["
			//<<*(currentNodePtr->subnetPtr->getBinaryRepresentation())<<" - "
			<<*(currentNodePtr->subnetPtr)<<"] ";
			//if(currentNodePtr->nextNodePtr!=0){
				//out<<" ";
			//}
			currentNodePtr=currentNodePtr->nextNodePtr;
		}
		return out;
	}

NetworkAddressNode::NetworkAddressNode(NetworkAddress *subnet, NetworkAddressNode * nextNode):
subnetPtr(subnet),
nextNodePtr(nextNode)
{

}
NetworkAddressNode::~NetworkAddressNode(){
	//do nothing
}


NetworkAddressSet::NetworkAddressSet():
size(0),
headNodePtr(0),
order(DESCENDING_PREFIX_LENGTH_ORDER)
{

}

NetworkAddressSet::~NetworkAddressSet()
{
	size=0;
	headNodePtr=0;
}

NetworkAddressSet * NetworkAddressSet::clone(){
	NetworkAddressSet *set=new NetworkAddressSet();
	set->sort(this->getOrder());
	NetworkAddressNode * currentNodePtr=headNodePtr;
	while(currentNodePtr!=0){
		set->insert(new NetworkAddress(*(currentNodePtr->subnetPtr)));
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	return set;
}

NetworkAddressSet * NetworkAddressSet::extract(int low, int subSize)throw(OutOfBoundException){
	if(low<0 || low>=size){
		throw OutOfBoundException("\"low\" argument to the extract() method is out of bounds");
	}
	if(subSize<=0 || (low+subSize)>getSize()){
		subSize=getSize()-low;
	}
	NetworkAddressSet *set=new NetworkAddressSet();
	set->sort(this->getOrder());
	for(int i=0;i<subSize;i++){
		set->insert(new NetworkAddress(*(getNetworkAddressAt(low+i))));
	}
	return set;
}

InetAddress NetworkAddressSet::getRandomAddress() throw(EmptyCollectionException){
	if(size<=0){
		throw EmptyCollectionException("NetworkAddsessSet object has no subnet addresses");
	}
	int position=rand()%size;
	NetworkAddress * tmp=getNetworkAddressAt(position);
	return tmp->getRandomAddress();
}

bool NetworkAddressSet::insert(NetworkAddress * subnet)throw(InvalidParameterException){
	bool inserted=false;
	if(subnet==0){
		throw InvalidParameterException("NetworkAddress pointer given to the insert method of NetworkAddressSet  is null");
	}
	if(!contains(*subnet)){
		//create a NetworkAddressNode object
		NetworkAddressNode *newNodePtr=new NetworkAddressNode(subnet,0);
		if(isEmpty()){
			headNodePtr=newNodePtr;
		}else{
			// we need first to locate the position to insert the newNodePtr.
			// but the correct location depends on the order of the set
			NetworkAddressNode *currentNodePtr=0, *prevNodePtr=0;
			if(order==ASCENDING_PREFIX_ORDER){
				for(currentNodePtr=headNodePtr;
				currentNodePtr!=0 && (currentNodePtr->subnetPtr->prefix < newNodePtr->subnetPtr->prefix);
				prevNodePtr=currentNodePtr, currentNodePtr=currentNodePtr->nextNodePtr);
			}else if(order==DESCENDING_PREFIX_LENGTH_ORDER){
				for(currentNodePtr=headNodePtr;
				currentNodePtr!=0 && (currentNodePtr->subnetPtr->prefixLength > newNodePtr->subnetPtr->prefixLength);
				prevNodePtr=currentNodePtr, currentNodePtr=currentNodePtr->nextNodePtr);
			}else if(order==ASCENDING_PREFIX_LENGTH_ORDER){
				for(currentNodePtr=headNodePtr;
				currentNodePtr!=0 && (currentNodePtr->subnetPtr->prefixLength < newNodePtr->subnetPtr->prefixLength);
				prevNodePtr=currentNodePtr, currentNodePtr=currentNodePtr->nextNodePtr);
			}else{
				for(currentNodePtr=headNodePtr;
				currentNodePtr!=0;
				prevNodePtr=currentNodePtr, currentNodePtr=currentNodePtr->nextNodePtr);
			}

			if(currentNodePtr==headNodePtr){//insert at the very start
				newNodePtr->nextNodePtr=currentNodePtr;
				headNodePtr=newNodePtr;
			}else if(currentNodePtr==0){//insert at the very end
				prevNodePtr->nextNodePtr=newNodePtr;
			}else{//insert at the middle
				prevNodePtr->nextNodePtr=newNodePtr;
				newNodePtr->nextNodePtr=currentNodePtr;
			}
		}
		size++;
		inserted=true;
	}
	return inserted;

}

NetworkAddress * NetworkAddressSet::removeNetworkAddressAt(int position)throw(OutOfBoundException){
	NetworkAddressNode *nodePtr=removeNodeAt(position);
	NetworkAddress * subnetPtr=nodePtr->subnetPtr;
	delete nodePtr;
	return subnetPtr;
}

NetworkAddress * NetworkAddressSet::removeNetworkAddress(NetworkAddress *toRemovePtr){
	int pos=-1;
	NetworkAddressNode *currentNodePtr=headNodePtr;
	for(int i=0;currentNodePtr!=0;i++){
		if(currentNodePtr->subnetPtr==toRemovePtr){
			pos=i;
			break;
		}
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	if(pos==-1){
		return 0;
	}else{
		return removeNetworkAddressAt(pos);
	}
}

NetworkAddress * NetworkAddressSet::removeNetworkAddress(NetworkAddress &toRemove){
	int pos=-1;
	NetworkAddressNode *currentNodePtr=headNodePtr;
	for(int i=0;currentNodePtr!=0;i++){
		if(*(currentNodePtr->subnetPtr)==toRemove){
			pos=i;
			break;
		}
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	if(pos==-1){
		return 0;
	}else{
		return removeNetworkAddressAt(pos);
	}
}

NetworkAddress * NetworkAddressSet::getNetworkAddressAt(int position)throw (OutOfBoundException){
	NetworkAddressNode *node=getNodeAt(position);
	return node->subnetPtr;
}

NetworkAddressNode * NetworkAddressSet::getNodeAt(int position)const throw (OutOfBoundException){
	if(position<0 || position>(size-1)){
		throw OutOfBoundException("NetworkAddressSet getElement() OutOfBoundException");
	}
	NetworkAddressNode *currentNodePtr=headNodePtr;
	for(int i=0;i<position;i++){
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	return currentNodePtr;
}

NetworkAddressNode * NetworkAddressSet::removeNodeAt(int position)throw (OutOfBoundException){
	if(position<0 || position>(size-1)){
		throw OutOfBoundException("NetworkAddressSet getElement() OutOfBoundException");
	}
	NetworkAddressNode *currentNodePtr=headNodePtr, *prevNodePtr=0;

	for(int i=0;i<position;i++){
		prevNodePtr=currentNodePtr;
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	if(currentNodePtr==headNodePtr){//remove very first element
		headNodePtr=currentNodePtr->nextNodePtr;
		currentNodePtr->nextNodePtr=0;
	}else{//remove middle or very last element
		prevNodePtr->nextNodePtr=currentNodePtr->nextNodePtr;
		currentNodePtr->nextNodePtr=0;
	}
	size--;
	return currentNodePtr;
}

void NetworkAddressSet::sort(NetworkAddressSetOrder newOrder){
	if(newOrder!=order && size>1){
		NetworkAddressNode *currentNodePtr=0;
		NetworkAddressNode *tmp;
		bool swapped;
		do{
			swapped=false;
			currentNodePtr=headNodePtr;
			while(currentNodePtr->nextNodePtr!=0){
				if(newOrder==DESCENDING_PREFIX_LENGTH_ORDER){
					if(currentNodePtr->subnetPtr->prefixLength < currentNodePtr->nextNodePtr->subnetPtr->prefixLength){
						tmp=currentNodePtr->nextNodePtr;
						if(swapNodes(currentNodePtr, currentNodePtr->nextNodePtr)){
							currentNodePtr=tmp;
						}
						swapped=true;
					}

				}else if(newOrder==ASCENDING_PREFIX_LENGTH_ORDER){
					if(currentNodePtr->subnetPtr->prefixLength > currentNodePtr->nextNodePtr->subnetPtr->prefixLength){
						tmp=currentNodePtr->nextNodePtr;
						if(swapNodes(currentNodePtr, currentNodePtr->nextNodePtr)){
							currentNodePtr=tmp;
						}
						swapped=true;
					}

				}else if(newOrder==ASCENDING_PREFIX_ORDER){
					if(currentNodePtr->subnetPtr->prefix > currentNodePtr->nextNodePtr->subnetPtr->prefix){
						tmp=currentNodePtr->nextNodePtr;
						if(swapNodes(currentNodePtr, currentNodePtr->nextNodePtr)){
							currentNodePtr=tmp;
						}
						swapped=true;
					}
				}
				currentNodePtr=currentNodePtr->nextNodePtr;
			}

		}while(swapped);
		order=newOrder;
	}
}

int * NetworkAddressSet::getPrefixFrequencyDistribution(){
	int arraySize=1 + (int)NetworkAddress::MAX_PREFIX_LENGTH;
	int * frequencyArray=new int[arraySize];
	for(int i=0; i<arraySize; i++){
		frequencyArray[i]=0;
	}
	NetworkAddressNode *currentNodePtr=headNodePtr;
	while(currentNodePtr!=0){
		frequencyArray[currentNodePtr->subnetPtr->prefixLength]++;
		currentNodePtr=currentNodePtr->nextNodePtr;
	}
	return frequencyArray;
}

bool NetworkAddressSet::swapNodes(NetworkAddressNode *n1, NetworkAddressNode *n2){
	bool swapped=false;
	if(n1!=0 && n2!=0){
		if(n1!=n2){
			NetworkAddressNode *prevAnteriorPtr=0, *prevPosteriorPtr=0, *currentNodePtr=headNodePtr, *prevNodePtr=0;
			int prevsFound=0;
			NetworkAddressNode *anteriorPtr=0, *posteriorPtr=0;
			while(currentNodePtr!=0 && prevsFound<2){
				if(currentNodePtr==n1){
					if(prevsFound==0){
						anteriorPtr=n1;
						prevAnteriorPtr=prevNodePtr;
					}else{
						posteriorPtr=n1;
						prevPosteriorPtr=prevNodePtr;
					}
					prevsFound++;
				}else if(currentNodePtr==n2){
					if(prevsFound==0){
						anteriorPtr=n2;
						prevAnteriorPtr=prevNodePtr;
					}else{
						posteriorPtr=n2;
						prevPosteriorPtr=prevNodePtr;
					}
					prevsFound++;
				}
				prevNodePtr=currentNodePtr;
				currentNodePtr=currentNodePtr->nextNodePtr;
			}
			if(prevsFound==2){

				NetworkAddressNode *tmp=0;

				if(prevAnteriorPtr!=0){
					prevAnteriorPtr->nextNodePtr=posteriorPtr;
				}

				tmp=posteriorPtr->nextNodePtr;

				if(anteriorPtr!=prevPosteriorPtr){//not adjacent
					posteriorPtr->nextNodePtr=anteriorPtr->nextNodePtr;
				}else{//adjacent
					posteriorPtr->nextNodePtr=anteriorPtr;
				}

				if(anteriorPtr!=prevPosteriorPtr){//not adjacent
					prevPosteriorPtr=anteriorPtr;
				}

				anteriorPtr->nextNodePtr=tmp;

				if(anteriorPtr==headNodePtr){
					headNodePtr=posteriorPtr;
				}
				swapped=true;
			}
		}else{//if n1==n2 assume that you have swapped the nodes and return true without really swapping
			swapped=true;
		}
	}
	return swapped;
}

bool NetworkAddressSet::contains(const NetworkAddress &subnet){
	if(order!=ASCENDING_PREFIX_ORDER){
		return containsLinear(subnet)>-1;
	}else{
		return containsBinary(subnet)>-1;
	}
}

int NetworkAddressSet::containsBinary(const NetworkAddress &subnet){
	int loc=-1;
	if(order!=ASCENDING_PREFIX_ORDER){
		loc=containsLinear(subnet);
	}else{
		if(size>0){
			int low=0;
			int high=size-1;
			NetworkAddressNode *midNode=0;
			int mid;
			while(low<=high){
				mid=(low+high)/2;
				midNode=getNodeAt(mid);
				if(*(midNode->subnetPtr)==subnet){
					loc=mid;
					break;
				}else if(subnet.prefix > midNode->subnetPtr->prefix){
					low=mid+1;
				}else{
					high=mid-1;
				}
			}
		}
	}
	return loc;
}
int NetworkAddressSet::containsLinear(const NetworkAddress &subnet){
	int loc=-1;
	if(size>0){
		NetworkAddressNode *currentNodePtr=headNodePtr;
		for(int i=0; currentNodePtr!=0 ; i++, currentNodePtr=currentNodePtr->nextNodePtr){
			if(*(currentNodePtr->subnetPtr)==subnet){
				loc=i;
				break;
			}
		}
	}
	return loc;
}

bool NetworkAddressSet::subsumes(const InetAddress &ip)const{
	NetworkAddress na(ip,NetworkAddress::MAX_PREFIX_LENGTH);//we treat an IP as a /32 subnet
	return this->subsumes(na);
}

bool NetworkAddressSet::subsumes(const NetworkAddress &subnet)const{
	if(order!=ASCENDING_PREFIX_ORDER){
		return subsumesLinear(subnet)>-1;
	}else{
		return subsumesBinary(subnet)>-1;
	}
}

int NetworkAddressSet::subsumesBinary(const InetAddress &ip)const{
	NetworkAddress na(ip,NetworkAddress::MAX_PREFIX_LENGTH);//we treat an IP as a /32 subnet
	int loc=-1;
	if(order!=ASCENDING_PREFIX_ORDER){
		loc=subsumesLinear(na);
	}else{
		loc=subsumesBinary(na);
	}
	return loc;
}
int NetworkAddressSet::subsumesLinear(const InetAddress &ip)const{
	NetworkAddress na(ip,NetworkAddress::MAX_PREFIX_LENGTH);//we treat an IP as a /32 subnet
	return subsumesLinear(na);
}
int NetworkAddressSet::subsumesBinary(const NetworkAddress &subnet)const{
	int loc=-1;
	if(order!=ASCENDING_PREFIX_ORDER){
		loc=subsumesLinear(subnet);
	}else{
		if(size>0){
			int low=0;
			int high=size-1;
			NetworkAddressNode *midNode=0;
			int mid;
			while(low<=high){
				mid=(low+high)/2;
				midNode=getNodeAt(mid);
				if(midNode->subnetPtr->subsumes(subnet)){
					loc=mid;
					break;
				}else if(subnet.prefix > midNode->subnetPtr->prefix){
					low=mid+1;
				}else{
					high=mid-1;
				}
			}
		}
	}
	return loc;
}
int NetworkAddressSet::subsumesLinear(const NetworkAddress &subnet)const{
	int loc=-1;
	if(size>0){
		NetworkAddressNode *currentNodePtr=headNodePtr;
		for(int i=0; currentNodePtr!=0 ; i++, currentNodePtr=currentNodePtr->nextNodePtr){
			if(currentNodePtr->subnetPtr->subsumes(subnet)){
				loc=i;
				break;
			}
		}
	}
	return loc;
}

void NetworkAddressSet::aggregate(){
	NetworkAddressSetOrder previousSetOrder=getOrder();
	sort(DESCENDING_PREFIX_LENGTH_ORDER);

	NetworkAddressNode *candidateNodePtr=headNodePtr;
	NetworkAddress *candidateSubnetToDelete;
	bool candidatePtrAlreadyUpdated=false;
	while(size>0 && candidateNodePtr->nextNodePtr!=0){
		NetworkAddressNode *targetNodePtr=candidateNodePtr->nextNodePtr;
		while(targetNodePtr!=0){
			if(targetNodePtr->subnetPtr->mergeAdjacent(*(candidateNodePtr->subnetPtr))){
				/*first remove and re-insert the target because it might be the next candidate after the removal of current candidate*/
				insert(removeNetworkAddress(targetNodePtr->subnetPtr));
				candidateSubnetToDelete=candidateNodePtr->subnetPtr;
				candidateNodePtr=candidateNodePtr->nextNodePtr;//after deletion this will be the next nodeptr
				candidatePtrAlreadyUpdated=true;
				delete removeNetworkAddress(candidateSubnetToDelete);
				break;
			}
			if(targetNodePtr->subnetPtr->subsumes(*(candidateNodePtr->subnetPtr))){
				candidateSubnetToDelete=candidateNodePtr->subnetPtr;
				candidateNodePtr=candidateNodePtr->nextNodePtr;//after deletion this will be the next nodeptr
				candidatePtrAlreadyUpdated=true;
				delete removeNetworkAddress(candidateSubnetToDelete);
				break;
			}
			targetNodePtr=targetNodePtr->nextNodePtr;
		}
		if(!candidatePtrAlreadyUpdated){
			candidateNodePtr=candidateNodePtr->nextNodePtr;
		}else{
			candidatePtrAlreadyUpdated=false;
		}
	}
	sort(previousSetOrder);
}

void NetworkAddressSet::clear(NetworkAddressSet &customerSet, bool verbose){
	NetworkAddressSetOrder previousSetOrder=getOrder();
	NetworkAddressSetOrder previousCustomerSetOrder=customerSet.getOrder();
	this->sort(DESCENDING_PREFIX_LENGTH_ORDER);
	customerSet.sort(DESCENDING_PREFIX_LENGTH_ORDER);
	//first aggregate the sets
	this->aggregate();
	customerSet.aggregate();
	//in order to remove or split larger blocks (smaller prefixLengths) reverse the customerSet
	customerSet.sort(ASCENDING_PREFIX_LENGTH_ORDER);
	NetworkAddressNode *customerCurrentNodePtr=customerSet.headNodePtr;
	NetworkAddress * targetSubnetPtr;
	bool deleted;
	float i=0;
	while(customerCurrentNodePtr!=0){
		deleted=false;
		targetSubnetPtr=customerCurrentNodePtr->subnetPtr;
		NetworkAddressNode * currenNodePtr=this->headNodePtr;
		NetworkAddress *candidateSubnetPtr;//the subnet to be deleted or splitted and partially deleted
		//cout<<"Target :\n"<<*targetSubnetPtr<<endl;
		while(currenNodePtr!=0){
			candidateSubnetPtr=currenNodePtr->subnetPtr;
			if(*candidateSubnetPtr==*targetSubnetPtr){//remove targetSubnetPtr
				//cout<<"**********Candidate to be directly deleted"<<endl<<*candidateSubnetPtr<<endl;
				delete removeNetworkAddress(*candidateSubnetPtr);
				//cout<<"major set after removal\n"<<*this<<endl;
				deleted=true;
				break;
			}
			if(candidateSubnetPtr->subsumes(*targetSubnetPtr)){
				//cout<<"major set before removal\n"<<*this<<endl;
				//cout<<"Candidate to be extracted and deleted"<<endl<<*candidateSubnetPtr<<endl;
				removeNetworkAddress(candidateSubnetPtr);
				NetworkAddressSet * splittedSet=candidateSubnetPtr->split(targetSubnetPtr->getPrefixLength());
				delete candidateSubnetPtr;
				//cout<<"major set after removal\n"<<*this<<endl;

				//cout<<"splitted set\n"<<*splittedSet<<endl;
				splittedSet->removeNetworkAddress(*targetSubnetPtr);
				//cout<<"splitted set after removal\n"<<*splittedSet<<endl;
				splittedSet->aggregate();
				//cout<<"splitted set after  re-aggregate\n"<<*splittedSet<<endl;
				NetworkAddressNode *tmp, *currentSplittedNodePtr=splittedSet->headNodePtr;
				while(currentSplittedNodePtr!=0){
					tmp=currentSplittedNodePtr;
					currentSplittedNodePtr=currentSplittedNodePtr->nextNodePtr;
					insert(tmp->subnetPtr);
					delete tmp;
				}
				delete splittedSet;
				//cout<<"major set after addition\n"<<*this<<endl;

				deleted=true;
				break;
			}
			currenNodePtr=currenNodePtr->nextNodePtr;
		}
		if(deleted==false){
			cout<<"ITOM Warning: Can NOT clear subnet"<<*(customerCurrentNodePtr->subnetPtr->getHumanReadableRepresentation())<<endl;
		}
		if(verbose){
			cout<<(++i/customerSet.size)*100<<" % of customer set has been processed"<<endl;
		}
		customerCurrentNodePtr=customerCurrentNodePtr->nextNodePtr;
	}
	customerSet.sort(previousCustomerSetOrder);
	this->sort(previousSetOrder);
}

void NetworkAddressSet::hardReleaseMemory(){
	while(size>0){
		delete removeNetworkAddressAt(0);
	}
}
void NetworkAddressSet::softReleaseMemory(){
	while(size>0){
		removeNetworkAddressAt(0);
	}
}


