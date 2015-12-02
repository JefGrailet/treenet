/*
 * IPTableEntry.h
 *
 *  Created on: Sep 30, 2015
 *      Author: grailet
 *
 * This file defines a class "IPTableEntry" which extends the InetAddress class. An entry from 
 * the IP Look-up Table (IPLookUpTable) is essentially an InetAddress with additionnal fields 
 * maintaining data related to the probing work, such as the protocol which should be used to 
 * probe this IP. The goal of the table is also to maintain this kind of information during the 
 * execution of TreeNET, as multiple InetAddress objects are used for a same IP and are not kept 
 * during next steps, therefore not suited for keeping such details.
 */

#ifndef IPTABLEENTRY_H_
#define IPTABLEENTRY_H_

#include <string>
using std::string;

#include "../../common/date/TimeVal.h"
#include "../../common/inet/InetAddress.h"

class IPTableEntry : public InetAddress
{
public:

    // Constants to represent uninitialized values
    const static unsigned char NO_KNOWN_TTL = (unsigned char) 255;
    const static unsigned long DEFAULT_TIMEOUT_SECONDS = 2;
    
    // Minimum amount of [Token, IP ID] pairs
    const static unsigned short MIN_ALIAS_RESOLUTION_PAIRS = 3;
    
    // Constructor, destructor
    IPTableEntry(InetAddress ip, unsigned short nbIPIDs);
    ~IPTableEntry();
    
    // Accessers/setters
    inline unsigned char getTTL() { return this->TTL; }
    inline TimeVal getPreferredTimeout() { return this->preferredTimeout; }
    inline void setTTL(unsigned char TTL) { this->TTL = TTL; }
    inline void setPreferredTimeout(TimeVal timeout) { this->preferredTimeout = timeout; }
    
    // Comparison method for sorting purposes
    static bool compare(IPTableEntry *ip1, IPTableEntry *ip2);
    
    // Regarding alias resolution
    bool hasIPIDData();
    
	inline void setProbeToken(unsigned short index, unsigned long pt) { this->probeTokens[index] = pt; }
	inline void setIPIdentifier(unsigned short index, unsigned short ii) { this->IPIdentifiers[index] = ii; }
	inline void setDelay(unsigned short index, unsigned long d) { this->delays[index] = d; }
	inline void setStoredHostName(string hn) { this->storedHostName = hn; }
	inline void setVelocityLowerBound(double vlb) { this->velocityLowerBound = vlb; }
	inline void setVelocityUpperBound(double vub) { this->velocityUpperBound = vub; }
	
	inline unsigned long getProbeToken(unsigned short index) { return this->probeTokens[index]; }
	inline unsigned short getIPIdentifier(unsigned short index) { return this->IPIdentifiers[index]; }
	inline unsigned long getDelay(unsigned short index) { return this->delays[index]; }
	inline string getStoredHostName() { return this->storedHostName; }
	inline double getVelocityLowerBound() { return this->velocityLowerBound; }
	inline double getVelocityUpperBound() { return this->velocityUpperBound; }
	
	// toString() method, for output purposes
    string toString();

private:
    unsigned char TTL;
    TimeVal preferredTimeout;
    
    // Alias resolution hints
    unsigned short nbIPIDs;
    unsigned long *probeTokens;
	unsigned short *IPIdentifiers;
	unsigned long *delays;
	string storedHostName;
	
	// Velocity of IP ID counter
	double velocityLowerBound, velocityUpperBound;
};

#endif /* IPTABLEENTRY_H_ */
