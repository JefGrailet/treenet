/*
 * VantagePointSelector.h
 *
 *  Created on: Feb 16, 2016
 *      Author: grailet
 *
 * TreeNET Reader v2.1 onwards now include a "transplant" feature which allows to build a single 
 * full network tree from distinct datasets obtained from distinct vantage points. It consists in 
 * using the interfaces occurring on the last steps of the routes of each dataset, as they should 
 * be the same from one dataset to another.
 *
 * The challenge of this feature is to select the best vantage point for this task, i.e. the 
 * dataset which the router interfaces occurring after the main trunk (after building a network 
 * tree) in the routes matches a maximum of routes from the other datasets. When several datasets 
 * share the maximum, the one with no hole (or rather, the one where the trunk does not contain a 
 * node which the only label is 0.0.0.0) and the smallest trunk is preferred over the others.
 */

#ifndef VANTAGEPOINTSELECTOR_H_
#define VANTAGEPOINTSELECTOR_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "./TreeNETEnvironment.h"
#include "./SubnetParser.h"
#include "../structure/SubnetSiteSet.h"
#include "../structure/NetworkTree.h"
#include "../structure/IPLookUpTable.h"

class VantagePointSelector
{
public:

    /* 
     * Constructor/destructor. Parameters for the constructor are:
     * -env, the TreeNETEnvironment object,
     * -nbVPs, the amount of recognized datasets (= from distinct VPs),
     * -subnetDumps, a pointer to an array of strings which are paths to the subnet dumps (these 
     *  paths are normally checked prior to the instantiation of this class).
     */
    
    VantagePointSelector(TreeNETEnvironment *env, list<string> dumpPaths);
    ~VantagePointSelector();
    
    // Method to launch the selection process
    void select();
    
    // Methods to retrieve the final results
    inline NetworkTree *getStartTree() { return this->startTree; }
    
private:

    // Pointer to the environment object
    TreeNETEnvironment *env;
    
    // Private fields
    unsigned short nbVPs;
    string *subnetDumpPaths;
    SubnetSiteSet **subnetSets;
    unsigned short *trunkHeight;
    list<InetAddress> *lateInterfaces;
    bool *hasHole;
    unsigned int *score;
    
    // Field to get the tree that actually starts the merging (built at the end of select())
    NetworkTree *startTree;
    
};

#endif /* VANTAGEPOINTSELECTOR_H_ */
