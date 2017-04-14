/*
 * Grafter.cpp
 *
 *  Created on: Nov 17, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Grafter.h (see this file to learn further about the 
 * goals of such class).
 */

#include <list>
using std::list;
#include <string>
using std::string;
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <sys/stat.h> // For CHMOD edition in saveScrappedSubnets()

#include "Grafter.h"
#include "../../../utils/SubnetParser.h"

Grafter::Grafter(TreeNETEnvironment *env, list<string> setList)
{
    this->env = env;
    nbSets = setList.size();
    
    SubnetParser *sp = new SubnetParser(env);
    unsigned short successfullyParsed = 0;
    
    buildingSets = new SubnetSiteSet*[nbSets];
    filePaths = new string[nbSets];
    int j = 0;
    for(list<string>::iterator i = setList.begin(); i != setList.end(); i++)
    {
        string curPath = (*i);
        filePaths[j] = curPath;
        buildingSets[j] = new SubnetSiteSet();
        
        // Parses the file content        
        bool res = sp->parse(curPath + ".subnet", buildingSets[j]);
        
        ostream *out = env->getOutputStream();
        if(res && buildingSets[j]->getNbSubnets() > 0)
        {
            successfullyParsed++;
            (*out) << "Successfully parsed " << curPath << ".\n" << endl;
        }
        else
        {
            (*out) << "Could not parse anything right in " << curPath << ".\n" << endl;
        }
        
        j++;
    }
    
    delete sp;
    
    // When less than 2 datasets successfully parsed: deletes everything and throws and exception
    if(successfullyParsed <= 2)
    {
        delete[] filePaths;
    
        for(unsigned short i = 0; i < nbSets; i++)
        {
            delete buildingSets[i];
            buildingSets[i] = NULL;
        }
        delete[] buildingSets;
    
        throw BadInputException();
    }
    
    // mainSetIndex is set to the first index of a non-empty subnet set
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(buildingSets[i]->getNbSubnets() > 0)
        {
            mainSetIndex = i;
            break;
        }
    }
    mainTrunkSize = 0; // For now
}

Grafter::~Grafter()
{
    delete[] filePaths;

    for(unsigned short i = 0; i < nbSets; i++)
    {
        delete buildingSets[i];
        buildingSets[i] = NULL;
    }
    delete[] buildingSets;
    
    while(scrappedSubnets.size() > 0)
    {
        SubnetSite *cur = scrappedSubnets.front();
        scrappedSubnets.pop_front();
        delete cur;
    }
}

unsigned short Grafter::getTrunkSize(NetworkTree *tree)
{
    unsigned short size = 0;
    NetworkTreeNode *cur = tree->getRoot();
    while(cur != NULL && !cur->isLeaf() && cur->getChildren()->size() == 1)
    {
        cur = cur->getChildren()->front();
        size++;
    }
    return size;
}

bool Grafter::isTrunkIncomplete(NetworkTree *tree)
{
    NetworkTreeNode *cur = tree->getRoot();
    while(cur != NULL && !cur->isLeaf() && cur->getChildren()->size() == 1)
    {
        cur = cur->getChildren()->front();
        if(cur->getLabels()->size() == 1 && cur->getLabels()->front() == InetAddress(0))
            return true;
    }
    return false;
}

list<InetAddress> Grafter::listLateInterfaces(NetworkTree *tree)
{
    list<InetAddress> result;

    // Gets to end of trunk
    NetworkTreeNode *trunkEnd = tree->getRoot();
    while(trunkEnd != NULL && !trunkEnd->isLeaf() && trunkEnd->getChildren()->size() == 1)
    {
        trunkEnd = trunkEnd->getChildren()->front();
    }
    
    // Returns empty list in case of problem or single-leaf tree
    if(trunkEnd == NULL || trunkEnd->isLeaf())
        return result;
    
    listLateInterfacesRecursive(trunkEnd, &result);
    result.sort(InetAddress::smaller);
    
    // Removes potential duplicates (rare but possible)
    InetAddress previous(0);
    for(list<InetAddress>::iterator i = result.begin(); i != result.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == previous)
            result.erase(i--);
        else
            previous = cur;
    }
    
    return result;
}

void Grafter::listLateInterfacesRecursive(NetworkTreeNode *cur, list<InetAddress> *res)
{
    // Stops if it is a leaf
    if(cur->isLeaf())
        return;

    // Goes through children and finds internals among them
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        NetworkTreeNode *curChild = (*i);
        if(curChild->isInternal())
        {
            list<InetAddress> *labels = curChild->getLabels();
            for(list<InetAddress>::iterator j = labels->begin(); j != labels->end(); ++j)
            {
                InetAddress interface = (*j);
                if(interface != InetAddress(0))
                {
                    res->push_back(interface);
                }
            }
            
            // Goes deeper
            listLateInterfacesRecursive((*i), res);
        }
    }
}

void Grafter::nullifyLeaves(NetworkTree *tree)
{
    nullifyLeavesRecursive(tree->getRoot());
}

void Grafter::nullifyLeavesRecursive(NetworkTreeNode *cur)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        NetworkTreeNode *cur = (*i);
        if(cur->isLeaf())
            cur->nullifySubnet();
        else
            nullifyLeavesRecursive(cur);
    }
}

void Grafter::selectRootstock()
{
    // Some useful arrays to later elect the best "rootstock"
    unsigned short *trunkHeight = new unsigned short[nbSets];
    list<InetAddress> *lateInterfaces = new list<InetAddress>[nbSets];
    bool *isIncomplete = new bool[nbSets];
    unsigned int *score = new unsigned int[nbSets];
    
    for(unsigned short i = 0; i < nbSets; i++)
    {
        trunkHeight[i] = 0;
        isIncomplete[i] = false;
        score[i] = 0;
    }
    
    // Building a tree for each dataset
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(buildingSets[i]->getNbSubnets() == 0)
            continue;
    
        env->copySubnetSet(buildingSets[i]);
        
        Grower *grower = new GrafterGrower(env);
        grower->grow();
        
        Soil *result = grower->getResult();
        NetworkTree *tree = result->getRootsList()->front();
        
        delete grower;
        
        trunkHeight[i] = getTrunkSize(tree);
        isIncomplete[i] = isTrunkIncomplete(tree);
        lateInterfaces[i] = listLateInterfaces(tree);
        
        nullifyLeaves(tree);
        delete result;
    }
    
    // Now seeing how many collisions occur with the late interfaces of other sets (for each set)
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(buildingSets[i]->getNbSubnets() == 0)
            continue;
    
        IPLookUpTable *tempIPDict = new IPLookUpTable(env->getNbIPIDs());
        
        for(unsigned short j = 0; j < nbSets; j++)
        {
            if(i == j)
                continue;
            
            list<InetAddress> list = lateInterfaces[j];
            for(std::list<InetAddress>::iterator it = list.begin(); it != list.end(); ++it)
                tempIPDict->create((*it)); // No need to check result here
        }
        
        list<InetAddress> list = lateInterfaces[i];
        unsigned short nbCollisions = 0;
        for(std::list<InetAddress>::iterator it = list.begin(); it != list.end(); ++it)
        {
            IPTableEntry *res = tempIPDict->create((*it)); // Checking is needed here
            if(res == NULL)
                nbCollisions++;
        }
        
        score[i] = nbCollisions;
        delete tempIPDict;
    }
    
    // Summarizing the computed data in the console
    ostream *out = env->getOutputStream();
    for(unsigned short i = 0; i < nbSets; i++)
    {
        (*out) << this->filePaths[i] << ": " << score[i];
        (*out) << " matches with other sets, trunk of height " << trunkHeight[i];
        if(isIncomplete[i])
            (*out) << " and incomplete";
        else
            (*out) << " and complete";
        (*out) << "\n";
    }
    (*out) << endl;
    
    unsigned int bestScore = 0;
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(!isIncomplete[i])
        {
            if(score[i] > bestScore)
            {
                this->mainSetIndex = i;
                this->mainTrunkSize = trunkHeight[i];
                bestScore = score[i];
            }
        }
    }
    (*out) << "TreeNET Forester has selected " << this->filePaths[this->mainSetIndex];
    (*out) << " to start the grafting.\n" << endl;
    
    delete[] trunkHeight;
    delete[] isIncomplete;
    delete[] lateInterfaces;
    delete[] score;
}

Soil* Grafter::growAndGraft()
{
    // Determines the required maxDepth for the final tree
    unsigned short maxRouteLength = 0;
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(i == mainSetIndex)
            continue;
    
        unsigned short curLength = buildingSets[i]->getMaximumDistance();
        if(curLength > maxRouteLength)
            maxRouteLength = curLength;
    }

    // Grows rootstock
    env->transferSubnetSet(buildingSets[mainSetIndex]);
    
    GrafterGrower *grower = new GrafterGrower(env, mainTrunkSize + maxRouteLength);
    grower->grow();
    
    // But all other sets in the main subnet set
    for(unsigned short i = 0; i < nbSets; i++)
    {
        if(i != mainSetIndex && buildingSets[i]->getNbSubnets() > 0)
        {
            env->transferSubnetSet(buildingSets[i]);
        }
    }
    env->getSubnetSet()->sortSet();
    
    // Actual grafting
    SubnetSiteSet *mainSet = env->getSubnetSet();
    SubnetSite *toInsert = mainSet->getValidSubnet();
    while(toInsert != NULL)
    {
        bool success = grower->graft(toInsert);
        if(!success)
            scrappedSubnets.push_back(toInsert);
    
        toInsert = mainSet->getValidSubnet();
        if(toInsert == NULL)
            toInsert = mainSet->getValidSubnet(false);
    }
    grower->flushMapEntries();
    
    Soil *finalResult = grower->getResult();
    delete grower;

    return finalResult;
}

void Grafter::outputScrappedSubnets(string filename)
{
    if(scrappedSubnets.size() > 0)
    {
        string output = "";
        for(list<SubnetSite*>::iterator i = scrappedSubnets.begin(); i != scrappedSubnets.end(); ++i)
        {
            SubnetSite *ss = (*i);
            string cur = ss->toString();
            
            if(!cur.empty())
                output += cur + "\n";
        }
        
        ofstream newFile;
        newFile.open(filename.c_str());
        newFile << output;
        newFile.close();
        
        // File must be accessible to all
        string path = "./" + filename;
        chmod(path.c_str(), 0766);
    }
}
