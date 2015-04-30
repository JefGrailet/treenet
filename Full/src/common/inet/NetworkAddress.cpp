

#include <cstdlib>
#include <cstring>
#include <iostream>
using std::cout;
using std::endl;

#include "../utils/StringUtils.h"
#include "NetworkAddressSet.h"

#include "NetworkAddress.h"

const unsigned char NetworkAddress::MAX_PREFIX_LENGTH=31;

unsigned char NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(const InetAddress & localInetAddress)throw (InetAddressException){
	struct ifaddrs *interfaceIterator = 0, *interfaceList = 0;
	unsigned char prefixLength = 0;
	 if (getifaddrs (&interfaceList) < 0){
		 throw InetAddressException("Could NOT get list of interfaces via \"getifaddrs\"");
	 }

	 for (interfaceIterator = interfaceList; interfaceIterator!=0; interfaceIterator = interfaceIterator->ifa_next){

		 if(interfaceIterator->ifa_addr==0){
			 continue;
		 }
		 if (strcmp(interfaceIterator->ifa_name,"lo") == 0){
			 continue;
		 }

		 if (interfaceIterator->ifa_addr->sa_family == AF_INET){
			 struct sockaddr_in *la=(struct sockaddr_in *)(interfaceIterator->ifa_addr);
			 InetAddress add;
			 add.setInetAddress((unsigned long int)ntohl(la->sin_addr.s_addr));
			 if(add==localInetAddress){
				 struct sockaddr_in *netmask=(struct sockaddr_in *)(interfaceIterator->ifa_netmask);
				 unsigned long int u=(unsigned long int)ntohl(netmask->sin_addr.s_addr);
				 unsigned long int uCount;
				 uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
				 prefixLength = (unsigned char) (((uCount + (uCount >> 3)) & 030707070707) % 63);

			 }
		 }
	 }
	 freeifaddrs (interfaceList);
	 if(prefixLength==0){
			throw InetAddressException("Could NOT obtain local subnetwork netmask");
	 }
	 return prefixLength;
}

NetworkAddress * NetworkAddress::getLocalNetworkAddress(const InetAddress & localInetAddress)throw (InetAddressException){
	unsigned char localPrefixLength = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localInetAddress);
	NetworkAddress *na = new NetworkAddress(localInetAddress, localPrefixLength);
	return na;
}

NetworkAddress::NetworkAddress(const InetAddress & subnet, unsigned char prefixLen)throw (InetAddressException):
prefix(subnet),
prefixLength(prefixLen)
{
	if(prefixLen>NetworkAddress::MAX_PREFIX_LENGTH){
		throw InetAddressException("Invalid subnet prefix in NetworkAddress("+(*(subnet.getHumanReadableRepresentation()))+", "+StringUtils::Uchar2string(prefixLen)+")");
	}
	resetSubnetPrefix();
	srand(time(0));
}
NetworkAddress::NetworkAddress(const string & subnet)throw (InetAddressException):
prefixLength(31)
{
	if(subnet.length()>18){//18 comes from aaa.bbb.ccc.ddd/mn
		throw InetAddressException("Invalid subnet prefix in NetworkAddress(const string & subnet)");
	}
	char tmp[19];
	strcpy(tmp,subnet.c_str());
	char * token=strtok(tmp, "/");
	prefix=InetAddress(token);
	token=strtok(0,"/");
	setSubnetPrefixLength((char)atoi(token));
	srand(time(0));
}

NetworkAddress::NetworkAddress(const NetworkAddress &subnet){
	this->prefix=subnet.prefix;
	this->prefixLength=subnet.prefixLength;
}

NetworkAddress::~NetworkAddress() {
	// TODO Auto-generated destructor stub
}
void NetworkAddress::setSubnetPrefix(const InetAddress & subnet){
	this->prefix=subnet;
	resetSubnetPrefix();
}
void NetworkAddress::setSubnetPrefixLength(unsigned char prefixLength)throw (InetAddressException){
	if(prefixLength>NetworkAddress::MAX_PREFIX_LENGTH){
		throw InetAddressException("Invalid subnet prefix length ");
	}
	if(this->prefixLength<prefixLength){//101.78.0.0/8 cannot be downcasted to /16 because we don't know value of byte # 3 (which is zero in the example)
		throw InetAddressException("Downcasting the subnet prefix causes loss in subnet prefix address");
	}

	this->prefixLength=prefixLength;
	resetSubnetPrefix();

}
void NetworkAddress::resetSubnetPrefix(){
	//make the host portion all zeros
	unsigned long int tmpAddr=prefix.getULongAddress();
	tmpAddr=tmpAddr>>(32-prefixLength);
	tmpAddr=tmpAddr<<(32-prefixLength);
	prefix.setInetAddress(tmpAddr);
}
auto_ptr<string> NetworkAddress::getHumanReadableRepresentation()const{
	auto_ptr<string> tmp=this->prefix.getHumanReadableRepresentation();
	auto_ptr<string> rep(new string(*tmp+string("/")+StringUtils::Uchar2string(prefixLength)));
	return rep;
}

auto_ptr<string> NetworkAddress::getBinaryRepresentation()const{
	auto_ptr<string> tmp=this->prefix.getBinaryRepresentation();
	auto_ptr<string> rep(new string(*tmp+" "+*(this->getHumanReadableRepresentation())));
	return rep;

}

bool NetworkAddress::subsumes(const InetAddress & ip)const{
	unsigned long int tmpThis=this->prefix.getULongAddress()>>(32-prefixLength);
	unsigned long int tmpIP=ip.getULongAddress()>>(32-prefixLength);
	return (tmpThis ^ tmpIP)==0;
}
bool NetworkAddress::subsumes(const NetworkAddress &subnet)const{
	if(subnet.prefixLength < this->prefixLength){
		return false;
	}else if(subnet.prefixLength==this->prefixLength){
		return (this->prefix)==subnet.prefix;
	}else{
		unsigned long int tmpThis=this->prefix.getULongAddress()>>(32-(this->prefixLength));
		unsigned long int tmpSubnet=subnet.prefix.getULongAddress()>>(32-(this->prefixLength));
		return (tmpThis ^ tmpSubnet)==0;
	}
}
InetAddress NetworkAddress::getRandomAddress()const{
	unsigned long int randomAddr;
	unsigned char * randomAddrPtr=(unsigned char *) &randomAddr;
	int randomPortionLength=((31-prefixLength)/8) +1;
	for(int i=0;i<randomPortionLength;i++){
		randomAddrPtr[i]=rand()%256;
	}

	randomAddr=randomAddr<<prefixLength;
	randomAddr=randomAddr>>prefixLength;
	return InetAddress(prefix.getULongAddress() | randomAddr);
}
InetAddress NetworkAddress::getLowerBorderAddress()const{
	return prefix;
}
InetAddress NetworkAddress::getUpperBorderAddress()const{
	unsigned long int addr=~0;
	addr=addr>>prefixLength;
	return InetAddress(prefix.getULongAddress() ^ addr);
}

int NetworkAddress::isBorder(const InetAddress & ip)const{
	//check lower border
	if(ip==prefix){
		return 1;
	}
	//check upper border
	unsigned long int tmp=~0;
	tmp=tmp>>prefixLength;
	if((ip.getULongAddress())==(prefix.getULongAddress() ^ tmp)){
		return 2;
	}
	//if both borders are false return false
	return 0;
}
bool NetworkAddress::isAdjacent(const NetworkAddress &subnet)const{
	if(this->prefixLength!=subnet.prefixLength){
		return false;
	}

	unsigned long tmpThis=(this->prefix.getULongAddress())>>(32-prefixLength);//first 32-prefixLength+1 bits aligned to right
	unsigned long tmpSubnet=(subnet.prefix.getULongAddress())>>(32-prefixLength);
	return (tmpThis ^ tmpSubnet)==1;

}
NetworkAddress NetworkAddress::getAdjacent()const{
	unsigned long int mask=1<<(32-prefixLength);
	unsigned long int tmpPrefix=prefix.getULongAddress() ^ mask;
	return NetworkAddress(InetAddress(tmpPrefix),prefixLength);
}
bool NetworkAddress::mergeAdjacent(const NetworkAddress &subnet){
	if(!(this->isAdjacent(subnet))){
		return false;
	}else{
		this->prefixLength--;
		resetSubnetPrefix();
		return true;
	}
}

NetworkAddressSet * NetworkAddress::split(unsigned char splitPrefixLength) throw(InvalidParameterException){
	if(this->prefixLength>=splitPrefixLength || splitPrefixLength>NetworkAddress::MAX_PREFIX_LENGTH){
		throw InvalidParameterException("Can NOT split a subnet to a prefixLength less than or equal to itself");
	}
	/**
	 * The idea is a subnet address /k in binary is xxxxxxxxxxxx|yyyyyyy where x's are prefix and Y's are
	 * suffix which is 0 in reality. To divide it into /m we just have to modify middle m-k bits starting
	 * from xxxxxxxxx|000|yyyy, xxxxxxxxx|001|yyyy, .... to xxxxxxxxx|111|yyyy.
	 */
	unsigned long increment=1<<(32-splitPrefixLength);//which is equal to pow(2, (32-splitPrefixLength))
	unsigned long iterationCount=1<<(splitPrefixLength-this->prefixLength);//which is equal to pow(2, (prefixLength-splitPrefixLength))
	NetworkAddressSet *nas=new NetworkAddressSet();
	//cout<<"increment="<<increment<<endl<<"iteratonCount="<<iterationCount<<endl;
	unsigned long ip=prefix.getULongAddress();
	unsigned long i=0;
	while(i<iterationCount){
		nas->insert(new NetworkAddress(InetAddress(ip),splitPrefixLength));
		i++;
		ip+=increment;
	}

	return nas;

}

