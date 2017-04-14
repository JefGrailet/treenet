/*
 * IPIDTuple.h
 *
 *  Created on: Feb 29, 2016
 *      Author: jefgrailet
 *
 * Provides together an IP-ID, its associated token, the time at which it was obtained (as a 
 * timeval object, to get precision in microseconds), a boolean value telling if the IP ID from 
 * the reply ICMP packet is the same as in the initial probe and the TTL value found in the reply. 
 * The interest of having a whole object is to be able to sort triplets.
 *
 * It was slightly extended in early April 2017 for the needs of a new IP-ID collection scheduling 
 * strategy.
 */

#ifndef IPIDTUPLE_H_
#define IPIDTUPLE_H_

#include <sys/time.h> // For gettimeofday()

#include "../../common/inet/InetAddress.h"

class IPIDTuple
{
public:

    // Constructors, destructor
    IPIDTuple();
    IPIDTuple(InetAddress target);
    ~IPIDTuple();
    
    // Fields are public, for the sake of simplicity
    InetAddress target;
    unsigned long int probeToken;
    unsigned short IPID;
    timeval timeValue;
    bool echo;
    unsigned char replyTTL;
    
    // Equality method
    IPIDTuple &operator=(const IPIDTuple &other);
    
    // Comparison methods for sorting purposes
    inline static bool compare(IPIDTuple &tuple1, IPIDTuple &tuple2) { return tuple1.probeToken < tuple2.probeToken; }
    inline static bool compareByID(IPIDTuple &tuple1, IPIDTuple &tuple2) { return tuple1.IPID < tuple2.IPID; }
    static bool compareByTime(IPIDTuple &tuple1, IPIDTuple &tuple2); // Latest time first

};

#endif /* IPIDTUPLE_H_ */
