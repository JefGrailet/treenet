/*
 * Ant.h
 *
 *  Created on: Nov 24, 2016
 *      Author: jefgrailet
 *
 * This climber class visits network trees to compute statistics over the inferred subnets and the 
 * neighborhoods.
 */

#ifndef ANT_H_
#define ANT_H_

#include <string>
using std::string;

#include "Climber.h"

class Ant : public Climber
{
public:

    // Constructor, destructor
    Ant(TreeNETEnvironment *env);
    ~Ant(); // Implicitely virtual
    
    void climb(Soil *fromSoil); // Implicitely virtual
    
    // Method to flush the computer statistics into a string/an output text file
    string getStatisticsStr();
    void outputStatistics(string filename);

protected:

    // Field to maintain Soil while travelling (useful for the getSubnetContaining() method)
    Soil *soilRef;

    // Fields for the values used for statistics
    unsigned int nbSubnets, nbCredibleSubnets;
    unsigned int responsiveIPs, coveredIPs;
    
    unsigned int nbNeighborhoods;
    unsigned int nOnlyLeaves, nCompleteLinkage, nPartialLinkage;
    unsigned int nKnownLabels;
    
    unsigned int sizeBiggestFingerprintList;

    /*
     * Method to recursively "climb" the tree, node by node.
     */
    
    void climbRecursive(NetworkTreeNode *cur);
};

#endif /* ANT_H_ */
