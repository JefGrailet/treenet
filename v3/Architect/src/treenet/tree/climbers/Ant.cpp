/*
 * Ant.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Ant.h (see this file to learn further about the goals of such 
 * class).
 */

#include <list>
using std::list;
#include <sstream>
using std::stringstream;
#include <fstream>
using std::ofstream;

#include <sys/stat.h> // For CHMOD edition

#include "Ant.h"

Ant::Ant(TreeNETEnvironment *env) : Climber(env)
{
    nbSubnets = 0;
    nbCredibleSubnets = 0;
    responsiveIPs = 0;
    coveredIPs = 0;
    nbNeighborhoods = 0;
    nOnlyLeaves = 0;
    nCompleteLinkage = 0;
    nPartialLinkage = 0;
    nKnownLabels = 0;
}

Ant::~Ant()
{
}

void Ant::climb(Soil *fromSoil)
{
    this->soilRef = fromSoil;
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    
    if(roots->size() > 1)
    {
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            this->climbRecursive((*i)->getRoot());
        }
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot());
    }
}

void Ant::climbRecursive(NetworkTreeNode *cur)
{
    // Root case: goes deeper
    if(cur->isRoot())
    {
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i));
        }
    }
    // Leaf case: checks the subnet (amount of interfaces, capacity, credibility)
    else if(cur->isLeaf())
    {
        SubnetSite *ss = cur->getAssociatedSubnet();
        if(ss != NULL)
        {
            this->nbSubnets++;
            if(ss->isCredible())
                this->nbCredibleSubnets++;
            
            this->responsiveIPs += ss->getNbResponsiveIPs();
            this->coveredIPs += ss->getCapacity();
        }
    }
    // Internal case: checks the neighborhood (linkage, labels appearing elsewhere or not)
    else
    {
        this->nbNeighborhoods++;
        
        if(cur->hasOnlyLeavesAsChildren())
        {
            this->nOnlyLeaves++;
            this->nCompleteLinkage++;
        }
        else
        {
            // Checks linkage in depth; this is a simplified version of a part of the code of Cat.cpp
            list<NetworkTreeNode*> *children = cur->getChildren();
            list<NetworkTreeNode*> childrenL;
            list<NetworkTreeNode*> childrenI;
            for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
            {
                if((*i) == NULL)
                    continue;
                if((*i)->isLeaf())
                    childrenL.push_back((*i));
                else if((*i)->isInternal())
                    childrenI.push_back((*i));
            }
            
            size_t nbNeighborSubnets = childrenL.size();
            unsigned short countChildrenI = (unsigned short) childrenI.size();
            
            if(nbNeighborSubnets > 0)
            {
                for(list<NetworkTreeNode*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
                {
                    // Just in case (PlanetLab...)
                    if((*i) == NULL)
                        continue;
                    
                    SubnetSite *ss = (*i)->getAssociatedSubnet();
                    
                    // Just in case (PlanetLab...)
                    if(ss == NULL)
                        continue;

                    /*
                     * This part of code verifies that each child subnet is crossed to reach one or 
                     * several children which are internal nodes. This also checks if the node has partial 
                     * or complete linkage with its children.
                     */
                    
                    if(childrenI.size() > 0)
                    {
                        for(list<NetworkTreeNode*>::iterator j = childrenI.begin(); j != childrenI.end(); ++j)
                        {
                            // Just in case
                            if((*j) == NULL)
                                continue;
                        
                            list<InetAddress> *labels = (*j)->getLabels();
                            
                            // Idem
                            if(labels == NULL)
                                continue;
                            
                            for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                            {
                                // Due to Hedera's, countChildrenI can eventually rollover
                                if(ss->contains((*k)) && countChildrenI > 0)
                                    countChildrenI--;
                            }
                        }
                    }
                }
            }
            
            if(countChildrenI == 0)
                this->nCompleteLinkage++;
            else if(countChildrenI == 1)
                this->nPartialLinkage++;
        }
        
        // Checks the labels
        list<InetAddress> *labels = cur->getLabels();
        unsigned short nbLabels = labels->size();
        unsigned short appearingLabels = 0;
        for(list<InetAddress>::iterator l = labels->begin(); l != labels->end(); ++l)
        {
            SubnetMapEntry *container = this->soilRef->getSubnetContaining((*l));
            if(container != NULL)
                appearingLabels++;
        }
        
        if(appearingLabels == nbLabels)
        {
            this->nKnownLabels++;
        }
        
        // Goes deeper
        list<NetworkTreeNode*> *children = cur->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            this->climbRecursive((*i));
        }
    }
}

string Ant::getStatisticsStr()
{
    stringstream ss;

    ss << "Total of responsive IPs: " << responsiveIPs << "\n";
    ss << "Total of IPs covered by inferred subnets: " << coveredIPs << "\n";
    
    double ratioResponsiveToCovered = ((double) responsiveIPs / (double) coveredIPs) * 100;
    ss << "Ratio responsive to covered IPs: " << ratioResponsiveToCovered << "%\n";
    
    ss << "Total of inferred subnets: " << nbSubnets << "\n";
    ss << "Total of credible subnets: " << nbCredibleSubnets << "\n";
    
    double ratioCredibleSubnets = ((double) nbCredibleSubnets / (double) nbSubnets) * 100;
    ss << "Ratio of credible subnets: " << ratioCredibleSubnets << "%\n";
    
    double ratioOnlyLeaves = ((double) nOnlyLeaves / (double) nbNeighborhoods) * 100;
    double ratioCompleteLinkage = ((double) nCompleteLinkage / (double) nbNeighborhoods) * 100;
    double ratioPartialLinkage = ((double) nPartialLinkage / (double) nbNeighborhoods) * 100;
    double ratioKnownLabels = ((double) nKnownLabels / (double) nbNeighborhoods) * 100;
    
    ss << "Total of neighborhoods: " << nbNeighborhoods << "\n";
    ss << "Total of neighborhoods with only leaves: " << nOnlyLeaves << " (" << ratioOnlyLeaves << "%)\n";
    ss << "Total of neighborhoods with complete linkage: " << nCompleteLinkage << " (" << ratioCompleteLinkage << "%)\n";
    ss << "Total of neighborhoods with partial linkage: " << nPartialLinkage << " (" << ratioPartialLinkage << "%)\n";
    ss << "Total of neighborhoods which labels all appear in inferred subnets: " << nKnownLabels << " (" << ratioKnownLabels << "%)\n";
    
    return ss.str();
}

void Ant::outputStatistics(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << this->getStatisticsStr();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
