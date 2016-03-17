/*
 * IPIDTuple.h
 *
 *  Created on: Feb 29, 2016
 *      Author: grailet
 *
 * Provides together an IP-ID, its associated token, the time at which it was obtained (as a 
 * timeval object, to get precision in microseconds), a boolean value telling if the IP ID from 
 * the reply ICMP packet is the same as in the initial probe and the TTL value found in the reply. 
 * The interest of having a whole object is to be able to sort triplets.
 */

#ifndef IPIDTUPLE_H_
#define IPIDTUPLE_H_

#include <sys/time.h> // Also provides gettimeofday() for calling code

class IPIDTuple
{
public:

    // Constructor, destructor
    IPIDTuple(unsigned long int token, 
              unsigned short ID, 
              timeval time, 
              bool echo, 
              unsigned char replyTTL);
    ~IPIDTuple();
    
    // Fields are public, for the sake of simplicity
    unsigned long int probeToken;
    unsigned short IPID;
    timeval timeValue;
    bool echo;
    unsigned char replyTTL;
    
    // Comparison method for sorting purposes
    static bool compare(IPIDTuple *tuple1, IPIDTuple *tuple2);

};

#endif /* IPIDTUPLE_H_ */
