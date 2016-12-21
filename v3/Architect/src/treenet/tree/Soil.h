/*
 * Soil.h
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * As the name suggests, an instance of the Soil class is meant to home one or several (network) 
 * trees, as a growth method scheduled for TreeNET v3.0 can potentially produce several trees and 
 * not a single unified tree. The interface for the class is supposed to act as an entry point 
 * for the "climber" objects (i.e. visiting each tree to do alias resolution, print out the tree 
 * structure, etc.). Tree growth is carried out by "grower" objects, which have however to 
 * interact with Soil to get a tree ID and inserts them in the list of roots. The Soil class is 
 * also responsible for the subnet map, a data structure designed for fast subnet look-up using a 
 * single InetAddress (i.e., we look for the subnet encompassing it).
 */

#ifndef SOIL_H_
#define SOIL_H_

#include <list>
using std::list;

#include "SubnetMapEntry.h"

class Soil
{
public:

    /*
     * Size of the subnetMap array (array of lists), which is used for fast look-up of a subnet 
     * stored in a tree on the basis of an interface it should contain. Its size is based on the 
     * fact that no subnet of a prefix length shorter than /20 was ever found in the measurements. 
     * Therefore, the 20 first bits of any interface in a subnet are used to access a list of 
     * subnets sharing the same prefix in O(1), the list containing at most 2048 subnets (2048 
     * subnets of prefix length /31). This dramatically speeds up the look-up for a subnet stored 
     * in a tree (in comparison with a more trivial method where one visits the tree), at the 
     * cost of using more memory).
     */

    const static unsigned int SIZE_SUBNET_MAP = 1048576;

    // Constructor, destructor
    Soil();
    ~Soil();
    
    // Accessers
    inline list<NetworkTree*> *getRootsList() { return &this->roots; }
    
    // Inserts a new tree
    inline void insertTree(NetworkTree *t) { this->roots.push_back(t); }
    
    // Inserts a whole list of SubnetMapEntry objects and sorts all lists of the map
    void insertMapEntries(list<SubnetMapEntry*> entries);
    void sortMapEntries();
    
    // Gets a subnet inserted in a tree which contains the given input address (NULL if not found).
    SubnetMapEntry *getSubnetContaining(InetAddress needle);
    
    // Write all subnets stored in any tree in an output file of a given name
    void outputSubnets(string filename);
    
private:

    /*
     * Private fields: roots of each tree, subnet map.
     */
    
    list<NetworkTree*> roots;
    list<SubnetMapEntry*> *subnetMap;
};

#endif /* SOIL_H_ */
