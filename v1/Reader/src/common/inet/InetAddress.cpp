#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
using std::cout;
using std::endl;
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "InetAddress.h"
#include "../utils/StringUtils.h"
#include "NetworkAddress.h"

const unsigned long int InetAddress::MAX_IP4=~((unsigned long int)0);
const int InetAddress::ULONG_BIT_LENGTH=sizeof(unsigned long)*8;


InetAddress InetAddress::getFirstLocalAddress()throw (InetAddressException){
	 struct ifaddrs *interfaceIterator = 0, *interfaceList = 0;
	 InetAddress add;
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
			 add.ip=(unsigned long int)ntohl(la->sin_addr.s_addr);
			 break;
		 }
	 }
	 freeifaddrs (interfaceList);
	 if(add.getULongAddress()==0){
		 throw InetAddressException("Could NOT get a valid local IP address");
	 }
	 return add;
}

InetAddress InetAddress::getLocalAddressByInterfaceName(const string &iname)throw (InetAddressException){
	struct ifaddrs *interfaceIterator = 0, *interfaceList = 0;
	 InetAddress add;
	 if (getifaddrs (&interfaceList) < 0){
		 throw InetAddressException("Could NOT get list of interfaces via \"getifaddrs\"");
	 }

	 for (interfaceIterator = interfaceList; interfaceIterator!=0; interfaceIterator = interfaceIterator->ifa_next){
		 if(interfaceIterator->ifa_addr==0){
			 continue;
		 }
		 if (interfaceIterator->ifa_addr->sa_family == AF_INET){
			 if(strcmp(interfaceIterator->ifa_name,iname.c_str()) == 0){
				 struct sockaddr_in *la=(struct sockaddr_in *)(interfaceIterator->ifa_addr);
				 add.ip=(unsigned long int)ntohl(la->sin_addr.s_addr);
				 break;
			 }
		 }
	 }
	 freeifaddrs (interfaceList);
	 if(add.getULongAddress()==0){
		 throw InetAddressException("Could NOT get a valid local IP address");
	 }
	 return add;
}
auto_ptr<vector<InetAddress> > InetAddress::getLocalAddressList()throw (InetAddressException){
	 struct ifaddrs *interfaceIterator = 0, *interfaceList = 0;
	 auto_ptr<vector<InetAddress> > vec(new vector<InetAddress>());
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
			 add.ip=(unsigned long int)ntohl(la->sin_addr.s_addr);
			 (*vec).push_back(add);
		 }
	 }
	 freeifaddrs (interfaceList);
	 return vec;
}

InetAddress InetAddress::getAddressByHostName(const string &hostName)throw (InetAddressException){
	struct hostent * he;//it is always in network byte order
	he=gethostbyname(hostName.c_str());
	if(he==0){
		throw InetAddressException((StringUtils::int2string(h_errno))+string(" Could not get host information"));
	}
	uint32_t ip=((struct in_addr *)he->h_addr)->s_addr;
	if(ip==(unsigned long int)-1){
			throw InetAddressException("Could not convert hostent to 32bit network byte ordered IP Address");
	}
	InetAddress add;
	add.ip=(unsigned long int)(ntohl(ip));
	return add;
}
InetAddress InetAddress::getAddressByIPString(const string &stringIP)throw (InetAddressException){
	struct in_addr ip;
	if((inet_aton(stringIP.c_str(), &ip))==0){
		throw InetAddressException("Can not convert string \""+stringIP+"\" to a valid INETAddress");
	}
	InetAddress add;
	add.ip=(unsigned long int)(ntohl(ip.s_addr));
	return add;
}

InetAddress InetAddress::getRandomAddress(){
	static unsigned long int ZERO_BIT=0;
	static unsigned long int ONE_BIT=1;
	static double MEAN=0.5;

	unsigned long int randomIP=0;
	double uniformRN=0;
	srand(time(0));
	for(int i=0;i<32;i++){
		uniformRN=(double)rand()/(double)RAND_MAX;
		if(uniformRN<MEAN){
			randomIP |= ZERO_BIT;
		}else{
			randomIP |= ONE_BIT;
		}
		if(i<31){
			randomIP=randomIP<<1;
		}
	}


	return InetAddress(randomIP);

}

InetAddress InetAddress::getUnicastRoutableRandomAddress(){
	InetAddress ip;
	do{
		ip=InetAddress::getRandomAddress();
	}while(ip.isUnicastRoutableAddress()==false);

	return ip;

}

InetAddress::InetAddress(unsigned long int address) throw(InetAddressException)
{
	setInetAddress(address);
	this->probeToken = 0;
	this->IPIdentifier = 0;
	this->storedHostName = "";
}

InetAddress::InetAddress(const string &address) throw(InetAddressException)
{
	setInetAddress(address);
	this->probeToken = 0;
	this->IPIdentifier = 0;
	this->storedHostName = "";
}

InetAddress::InetAddress(const InetAddress &address)
{
	this->ip = address.ip;
	this->probeToken = address.probeToken;
	this->IPIdentifier = address.IPIdentifier;
	this->storedHostName = address.storedHostName;
}

InetAddress::~InetAddress()
{
	// TODO Auto-generated destructor stub
}

bool InetAddress::isUnicastRoutableAddress(){
	static NetworkAddress naArray[]={
			//private network address range
			NetworkAddress(InetAddress("10.0.0.0"),24),
			NetworkAddress(InetAddress("172.16.0.0"),20),
			NetworkAddress(InetAddress("192.168.0.0"),16),
			//link local address range
			NetworkAddress(InetAddress("169.254.0.0"),16),
			//multicast address range
			NetworkAddress(InetAddress("224.0.0.0"),24),
			NetworkAddress(InetAddress("232.0.0.0"),8),
			NetworkAddress(InetAddress("233.0.0.0"),8),
			NetworkAddress(InetAddress("239.0.0.0"),8)
	};
	static const int length=8;

	for(int i=0;i<length;i++){
		if(naArray[i].subsumes(*this)){
			return false;
		}
	}

	static InetAddress all255(~0);
	static InetAddress allzero(0);

	if(this->ip==all255.ip || this->ip==allzero.ip){
		return false;
	}

	return true;
}

InetAddress & InetAddress::operator=(const InetAddress &other)
{
    this->ip = other.ip;
    this->probeToken = other.probeToken;
    this->IPIdentifier = other.IPIdentifier;
    this->storedHostName = other.storedHostName;
    return *this;
}

void InetAddress::operator+=(unsigned int n)throw(InetAddressException){
	if((InetAddress::MAX_IP4 - this->ip)<=n){
		throw InetAddressException("4 bytes IP address range overflows");

	}
	this->ip+=n;
}
InetAddress & InetAddress::operator++()throw(InetAddressException){
	(*this)+=1;
	return *this;
}
InetAddress  InetAddress::operator++(int)throw(InetAddressException){
	(*this)+=1;
	return InetAddress(this->ip -1);
}
InetAddress InetAddress::operator+(unsigned int n)throw(InetAddressException){
	if((InetAddress::MAX_IP4 - this->ip)<=n){
		throw InetAddressException("4 bytes IP address range overflows");
	}
	return InetAddress(this->ip + n);
}
void InetAddress::operator-=(unsigned int n)throw(InetAddressException){
	if((this->ip - n)<0){
		throw InetAddressException("4 bytes IP address range underflows");
	}
	this->ip-=n;
}
InetAddress & InetAddress::operator--()throw(InetAddressException){
	(*this)-=1;
	return *this;
}
InetAddress  InetAddress::operator--(int)throw(InetAddressException){
	(*this)-=1;
	return InetAddress(this->ip +1);
}
InetAddress InetAddress::operator-(unsigned int n)throw(InetAddressException){
	if((this->ip - n)<0){
		throw InetAddressException("4 bytes IP address range underflows");
	}
	return InetAddress(this->ip - n);
}

void InetAddress::setBit(int position, int value){
	if(position>=0 && position<32){//valid position index
		if(value==1){//turn on a bit
			ip=(1<<(31-position)) | ip;
		}

		if(value==0){//turn off a bit
			ip=(~(1<<(31-position))) & ip;

		}
	}

}
int InetAddress::getBit(int position)const{
	if(position>=0 && position<32){//valid position index
		return (((1<<(31-position)) & (this->ip)))>0 ? 1 : 0;
	}else{
		return -1;
	}
}

void InetAddress::inverseBits(){
	static unsigned long ALL_ONES=~0;
	this->ip=this->ip ^ ALL_ONES;
}

void InetAddress::reverseBits(){
	unsigned long int x = this->ip;
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	this->ip = ((x >> 16) | (x << 16));
}


void InetAddress::setInetAddress(unsigned long int address)throw (InetAddressException){
	if(address>InetAddress::MAX_IP4 || address<0){
		throw InetAddressException("Can not convert the 4 byte into a valid INETAddress");
	}else{
		this->ip=address;
	}
}
void InetAddress::setInetAddress(const string &address)throw (InetAddressException){
	bool isHostName=false;
	for(unsigned int i=0;i <address.length(); i++){
		if(isalpha(address[i])){
			isHostName=true;
			break;
		}
	}
	InetAddress tmp;
	if(isHostName){
		tmp=InetAddress::getAddressByHostName(address);
	}else{
		tmp=InetAddress::getAddressByIPString(address);
	}
	this->ip=tmp.ip;
}
auto_ptr<string> InetAddress::getHumanReadableRepresentation()const{
	struct  in_addr addr;
	addr.s_addr=htonl(this->ip);
	auto_ptr<string> str(new string(inet_ntoa(addr)));
	return str;

}
auto_ptr<string> InetAddress::getBinaryRepresentation()const{
	auto_ptr<string> str(new string());

	for(long int i=31; i>=0; i--){
		if((1<<i) & (this->ip)){
			str->push_back('1');
		}else{
			str->push_back('0');
		}
		if(i!=0 && i%8==0){
			str->push_back('.');
		}
	}

	return str;
}
auto_ptr<string> InetAddress::getHostName()const{
	struct hostent * he;
	struct in_addr addr;
	addr.s_addr=inet_addr((*(getHumanReadableRepresentation())).c_str());
	he=gethostbyaddr((const char *) &addr.s_addr, sizeof(addr.s_addr), AF_INET);
	if(he!=0){
		auto_ptr<string> str(new string(he->h_name));
		return str;
	}else{
		auto_ptr<string> str(new string(""));
		return str;
	}
}

InetAddress InetAddress::get30Mate()const throw (InetAddressException){
	unsigned long int three=3;//000...00011
	unsigned long int tmp=three & this->ip;
	if(tmp==0 || tmp==3){
		throw InetAddressException("This IP can NOT have a /30 mate");
	}
	return InetAddress(three ^ this->ip);

}

unsigned long int InetAddress::getULongAddress()const{
	return this->ip;
}

