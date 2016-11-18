/*
 * IPLookUpTable.h
 *
 *  Created on: Sep 29, 2015
 *      Author: jefgrailet
 *
 * IPLookUpTable is a particular structure built to quickly look up for an IP address and see 
 * which protocol should be used to probe it (other usages could be added in the future, like 
 * obtaining the distance in TTL). All target IPs at the beginning of the program should be probed 
 * to determine which protocol they should be probed with during the next steps to maximize the 
 * amount of responsive IPs. IPs that are not responsive are not stored within the table for the 
 * sake of simplicity (and memory usage).
 *
 * The structure is made of a 8 Mio array of lists which are indexed from 0 to 2^X. An IP is 
 * stored in the structure by adding it to the list at the index which matches its X first bits. 
 * This implies that each list can contain at most 2^(32 - X) items, thanks to which the storage 
 * and look-up can be achieved in O(1) as long as X is great enough.
 *
 * The choice of X is free. By default, it is 20, which leads to a 8 Mio array of lists where each 
 * lists can contain up to 4096 elements. X can be adapted to benefit from large memory amounts in 
 * better computers.
 */

#ifndef IPLOOKUPTABLE_H_
#define IPLOOKUPTABLE_H_

#include <list>
using std::list;

#include "IPTableEntry.h"

class IPLookUpTable
{
public:
    // X = 20
    const static unsigned int SIZE_TABLE = 1048576;
    
    // Constructor, destructor
    IPLookUpTable(unsigned short nbIPIDs);
    ~IPLookUpTable();
    
    // Method to check if the dictionnary is empty or not
    bool isEmpty();
    
    // Creation and look-up methods
    IPTableEntry *create(InetAddress needle); // NULL if already existed
    IPTableEntry *lookUp(InetAddress needle); // NULL if not found
    
    // Output methods
    void outputDictionnary(string filename);
    void outputFingerprints(string filename);

private:
    list<IPTableEntry*> *haystack;
    unsigned short nbIPIDs;
};

#endif /* IPLOOKUPTABLE_H_ */
