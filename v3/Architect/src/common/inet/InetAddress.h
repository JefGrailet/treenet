/*
 * InetAddress.h
 *
 * This file defines a class "InetAddress" used to handle IPs in IPv4 format. The date at which 
 * it was written and the author are unknown (but see source below for more details). This class 
 * was already in ExploreNET and has been improved coding style-wise in September 2015 by J.-F. 
 * Grailet.
 */

#ifndef INETADDRESS_H_
#define INETADDRESS_H_
#include <iostream>
using std::ostream;
#include <netinet/in.h>
#include <ifaddrs.h>
#include <string>
using std::string;
#include <memory>
using std::auto_ptr;
#include <vector>
using std::vector;

#include "InetAddressException.h"
#include "../utils/StringUtils.h"

// VERY NICE SOURCE http://www.lemoda.net/freebsd/net-interfaces/index.html

class InetAddress
{
public:
	static const unsigned long int MAX_IP4;
	static const int ULONG_BIT_LENGTH;
	friend ostream &operator<<(ostream &out, const InetAddress &ip)
	{
		out << *(ip.getHumanReadableRepresentation()); /* +string("(")+StringUtils::Ulong2string(ip.getULongAddress())+string(")") */
		return out;
	}
	static auto_ptr<vector<InetAddress> > getLocalAddressList() throw(InetAddressException);
	static InetAddress getFirstLocalAddress() throw(InetAddressException);
	static InetAddress getLocalAddressByInterfaceName(const string &iname) throw(InetAddressException);
	static InetAddress getAddressByHostName(const string &hostName) throw(InetAddressException);
	static InetAddress getAddressByIPString(const string &stringIP) throw(InetAddressException);

	/**
	 * You need to call srand(time(NULL)) before using those two random functions if you want 
	 * different sequences.
	 */
	
	static InetAddress getRandomAddress();
	static InetAddress getUnicastRoutableRandomAddress();
	InetAddress(unsigned long int ip = 0) throw(InetAddressException);
	InetAddress(const string &address) throw(InetAddressException);
	InetAddress(const InetAddress &address);
	virtual ~InetAddress();
	bool isUnicastRoutableAddress(); // Returns true if the IP address is a unicast routable address
	InetAddress &operator=(const InetAddress &other);
	void operator+=(unsigned int n) throw(InetAddressException);
	InetAddress &operator++() throw(InetAddressException);
	InetAddress operator++(int) throw(InetAddressException);
	InetAddress operator+(unsigned int n) throw(InetAddressException);
	
	inline bool operator==(const InetAddress &other) const { return this->ip == other.ip; }
	inline bool operator!=(const InetAddress &other) const { return this->ip != other.ip; }
	inline bool operator<(const InetAddress &other) const { return this->ip < other.ip; }
	inline bool operator<=(const InetAddress &other) const { return this->ip <= other.ip; }
	inline bool operator>(const InetAddress &other) const { return this->ip > other.ip; }
	inline bool operator>=(const InetAddress &other) const { return this->ip >= other.ip; }
	
	void operator-=(unsigned int n) throw(InetAddressException);
	InetAddress &operator--() throw(InetAddressException);
	InetAddress operator--(int) throw(InetAddressException);
	InetAddress operator-(unsigned int n) throw(InetAddressException);
	void setBit(int position, int value); // 0 <= position <= 31
	int getBit(int position) const; // 0 <= position <= 31
	void inverseBits(); // Inverses each bit of the IP address e.g.  1011...11 will be 0100...00
	void reverseBits(); // Reverses the bit sequence of the IP address e.g. 1011...11 will be 11...1101
	auto_ptr<string> getHumanReadableRepresentation() const;
	auto_ptr<string> getBinaryRepresentation() const;
	auto_ptr<string> getHostName() const;
	unsigned long int getULongAddress() const;
	
	inline InetAddress get31Mate() const { return InetAddress(1 ^ this->ip); }
	inline bool is31Mate(const InetAddress &addr) const { return ((this->ip ^ addr.ip) == 1); }

	/**
	 * Throws InetAddressException if the IP ends with 00 or 11 binary
	 */
	
	InetAddress get30Mate() const throw(InetAddressException);
	void setInetAddress(unsigned long int address) throw(InetAddressException);
	void setInetAddress(const string &address) throw(InetAddressException);
	
	inline bool is30Mate(const InetAddress &addr) const { return ((this->ip ^ addr.ip) == 3); }
	inline bool isEnding00() const { return ((this->ip & 3) == 0); }
	inline bool isEnding11() const { return ((this->ip & 3) == 3); }
	inline bool isUnset() const { return (ip == 0); }
	inline void unSet() { this->ip = 0; }
	
	// Comparison method for sorting purposes (added by J.-F. Grailet)
	inline static bool smaller(InetAddress &inet1, InetAddress &inet2) { return inet1 < inet2; }

protected:
	unsigned long int ip;
};

#endif /* INETADDRESS_H_ */
