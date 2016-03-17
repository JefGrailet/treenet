/*
 * Fingerprint.h
 *
 *  Created on: Mar 7, 2016
 *      Author: grailet
 *
 * Gathers together 3 distinct values related to a same IP:
 * -the inferred initial "Echo reply" TTL,
 * -the type of IP ID counter (echo, random, healthy or "no idea"),
 * -the host name.
 * IPs with a similar fingerprint can be associated together easily, especially when the 
 * IP ID-based techniques are difficult to use. The main goal of having a whole class for it is 
 * to be able to sort those fingerprints according to their characteristics and group them 
 * together more easily.
 *
 * It is worth noting the host name, here, is only used for sorting purposes (i.e., for a group 
 * where the first two values are identical, the triplets with a DNS come first after sorting), 
 * and is only used as a way to chunk further large groups of IPs when host names do not match 
 * between two IPs.
 */

#ifndef FINGERPRINT_H_
#define FINGERPRINT_H_

#include "../structure/IPTableEntry.h"

class Fingerprint
{
public:

    // Print out method
    friend ostream &operator<<(ostream &out, const Fingerprint &f)
    {
        out << "<";
        if(f.initialTTL > 0)
            out << (unsigned short) f.initialTTL;
        else
            out << "*";
        out << ",";
        switch(f.IPIDCounterType)
        {
            case IPTableEntry::HEALTHY_COUNTER:
                out << "Healthy";
                break;
            case IPTableEntry::RANDOM_COUNTER:
                out << "Random";
                break;
            case IPTableEntry::ECHO_COUNTER:
                out << "Echo";
                break;
            default:
                out << "*";
                break;
        }
        out << ",";
        if(!f.hostName.empty())
            out << "Yes>";
        else
            out << "No>";
        return out;
    }

    // Constructor, destructor
    Fingerprint(IPTableEntry *ip);
    ~Fingerprint();
    
    // Fields are public, for the sake of simplicity
    IPTableEntry *ipEntry;
    unsigned char initialTTL;
    unsigned short IPIDCounterType;
    string hostName;
    
    // Comparison method for sorting purposes
    static bool compare(Fingerprint &f1, Fingerprint &f2);
    
    // Equality test, to see if consecutive fingerprints (after sorting) belongs to a same group
    bool equals(Fingerprint &f);
    
    // Method to tell if IPs with such a fingerprint should be grouped by default
    bool toGroupByDefault();

};

#endif /* FINGERPRINT_H_ */
