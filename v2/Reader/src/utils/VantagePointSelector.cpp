/*
 * VantagePointSelector.cpp
 *
 *  Created on: Feb 16, 2016
 *      Author: grailet
 *
 * Implements the class defined in VantagePointSelector.h (see this file to learn further about 
 * the goals of such class).
 */

#include <fstream>
using std::ifstream;

using namespace std;

#include "VantagePointSelector.h"

VantagePointSelector::VantagePointSelector(TreeNETEnvironment *env, list<string> dumpPaths)
{
    this->env = env;
    this->nbVPs = dumpPaths.size();
    
    this->subnetDumpPaths = new string[nbVPs];
    this->subnetSets = new SubnetSiteSet*[nbVPs];
    this->trunkHeight = new unsigned short[nbVPs];
    this->lateInterfaces = new list<InetAddress>[nbVPs];
    this->hasHole = new bool[nbVPs];
    this->score = new unsigned int[nbVPs];
    
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        this->subnetDumpPaths[i] = dumpPaths.front();
        this->subnetSets[i] = new SubnetSiteSet();
        this->trunkHeight[i] = 0;
        this->hasHole[i] = false;
        this->score[i] = false;
        
        dumpPaths.pop_front();
    }
    
    this->startTree = NULL;
}

VantagePointSelector::~VantagePointSelector()
{
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        delete this->subnetSets[i];
    }

    delete[] subnetDumpPaths;
    delete[] subnetSets;
    delete[] trunkHeight;
    delete[] lateInterfaces;
    delete[] hasHole;
    delete[] score;
}

void VantagePointSelector::select()
{
    ostream *out = env->getOutputStream();

    // Step 1.1: parsing and analyzing each file (max route length is also already computed)
    unsigned short maxRouteLength = 0;
    SubnetParser *sp = new SubnetParser(env);
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        SubnetSiteSet *curSet = this->subnetSets[i];
    
        // Step 1.1: parsing the subnet and storing in the right set
        string subnetDumpContent = "";
        ifstream inFileSubnet;
        inFileSubnet.open((this->subnetDumpPaths[i] + ".subnet").c_str());
        if(inFileSubnet.is_open())
        {
            subnetDumpContent.assign((std::istreambuf_iterator<char>(inFileSubnet)),
                                     (std::istreambuf_iterator<char>()));
            
            inFileSubnet.close();
        }
        else
        {
            // Should not occur, but if yes, next instruction will insert zero subnet
        }
        sp->parse(curSet, subnetDumpContent, true);
        
        (*out) << "Parsed " << this->subnetDumpPaths[i] << "." << endl;
        
        // Step 1.2: creating the corresponding network tree (first subnets with a complete route)
        unsigned short treeMaxDepth = curSet->getLongestRoute();
        if(maxRouteLength < treeMaxDepth)
            maxRouteLength = treeMaxDepth;
        curSet->sortByRoute();
        
        NetworkTree *tempTree = new NetworkTree(treeMaxDepth);
        
        SubnetSite *toInsert = curSet->getValidSubnet();
        while(toInsert != NULL)
        {
            tempTree->insert(toInsert);
            toInsert = curSet->getValidSubnet();
        }

        toInsert = curSet->getValidSubnet(false);
        while(toInsert != NULL)
        {
            tempTree->insert(toInsert);
            toInsert = curSet->getValidSubnet(false);
        }
        
        (*out) << "Finished building the tree for " << this->subnetDumpPaths[i] << ".\n" << endl;
        
        // Step 1.3: determining trunk size, interfaces after trunk and presence of holes
        this->trunkHeight[i] = tempTree->getTrunkSize();
        this->hasHole[i] = tempTree->hasIncompleteTrunk();
        this->lateInterfaces[i] = tempTree->listInterfacesAfterTrunk();
        
        (*out) << "Trunk height: " << this->trunkHeight[i] << "\n";
        if(this->hasHole[i])
            (*out) << "Presence of hole(s) in the trunk.\n";
        else
            (*out) << "No hole was found in the trunk.\n";
        (*out) << endl;
        
        // Freeing tree for next step
        tempTree->nullifyLeaves(curSet);
        delete tempTree;
    }
    
    delete sp;
    
    /*
     * Step 2: for each set, put all "late interfaces" from all other sets in a new IP dictionnary 
     * and see how many collisions occurs when we start inserting the "late interfaces" from the 
     * current set. This amount is stored in score[] afterwards.
     */
    
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        IPLookUpTable *tempIPDict = new IPLookUpTable(env->getNbIPIDs());
        
        for(unsigned short j = 0; j < nbVPs; j++)
        {
            if(i == j)
                continue;
            
            list<InetAddress> list = this->lateInterfaces[j];
            for(std::list<InetAddress>::iterator it = list.begin(); it != list.end(); ++it)
            {
                tempIPDict->create((*it)); // No need to check result here
            }
        }
        
        list<InetAddress> list = this->lateInterfaces[i];
        unsigned short nbCollisions = 0;
        for(std::list<InetAddress>::iterator it = list.begin(); it != list.end(); ++it)
        {
            IPTableEntry *res = tempIPDict->create((*it)); // Checking is needed here
            if(res == NULL)
            {
                nbCollisions++;
            }
        }
        
        this->score[i] = nbCollisions;
        delete tempIPDict;
    }
    
    // Step 3: summarizing the computed data in the console
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        (*out) << this->subnetDumpPaths[i] << ": " << this->score[i];
        (*out) << " matches with other sets, trunk of height " << this->trunkHeight[i];
        if(this->hasHole[i])
            (*out) << " and incomplete";
        else
            (*out) << " and complete";
        (*out) << "\n";
    }
    (*out) << endl;
    
    /*
     * Step 4: choosing the best "starting set", which is chosen as the set with a complete trunk 
     * that has the best score. A set with an incomplete trunk and a better score might exist, but 
     * it should be more careful to choose one with a complete trunk (missing interfaces can 
     * sometimes be the consequence of routing irregularities).
     */
    
    unsigned short selectedSetIndex = 0;
    unsigned short bestScore = 0;
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        if(!this->hasHole[i])
        {
            if(this->score[i] > bestScore)
            {
                selectedSetIndex = i;
                bestScore = this->score[i];
            }
        }
    }
    (*out) << "TreeNET Reader has selected " << this->subnetDumpPaths[selectedSetIndex];
    (*out) << " to start building the merged tree.\n" << endl;
    
    /*
     * Step 5 is building the final tree. But first, one has to choose an appropriate max depth 
     * for the final tree. It is computed as the trunk height of the selected set, plus the 
     * largest route length. It will probably lead to an overly large prediction of the tree 
     * depth, but this should prevent memory issues upon building the final tree.
     */
   
    unsigned short finalMaxDepth = this->trunkHeight[selectedSetIndex] + maxRouteLength;
    NetworkTree *finalTree = new NetworkTree(finalMaxDepth);
    SubnetSiteSet *selectedSet = this->subnetSets[selectedSetIndex];
        
    SubnetSite *toInsert = selectedSet->getValidSubnet();
    while(toInsert != NULL)
    {
        finalTree->insert(toInsert);
        toInsert = selectedSet->getValidSubnet();
    }

    toInsert = selectedSet->getValidSubnet(false);
    while(toInsert != NULL)
    {
        finalTree->insert(toInsert);
        toInsert = selectedSet->getValidSubnet(false);
    }
    
    /*
     * Final step is emptying the subnets sets to put all subnets into the main subnet set (the 
     * one to which env keeps a pointer). Same set is also sorted. This will finish setting 
     * everything for the transplant process (second and final main step of the merging mode).
     */
    
    list<SubnetSite*> *dst = env->getSubnetSet()->getSubnetSiteList();
    for(unsigned short i = 0; i < nbVPs; i++)
    {
        if(i == selectedSetIndex)
            continue;
        
        list<SubnetSite*> *src = this->subnetSets[i]->getSubnetSiteList();
        unsigned int maxSize = src->size();
        for(unsigned int j = 0; j < maxSize; j++)
        {
            SubnetSite *cur = src->front();
            src->pop_front();
            dst->push_back(cur);
        }
    }
    dst->sort(SubnetSite::compare);
    
    // The procedure stops by assigning the "start" tree to the dedicated field.
    this->startTree = finalTree;
}
