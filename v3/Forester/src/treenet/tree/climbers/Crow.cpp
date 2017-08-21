/*
 * Crow.cpp
 *
 *  Created on: Sept 13, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Crow.h (see this file to learn further about the goals of such 
 * class).
 */

#include <list>
using std::list;

// 3 next lines are for outputting data in text files
#include <fstream>
#include <sys/stat.h> // For CHMOD edition
using std::ofstream;

#include "Crow.h"

Crow::Crow(TreeNETEnvironment *env) : Climber(env)
{
    ar = new AliasResolver(env);
}

Crow::~Crow()
{
    delete ar;
}

void Crow::climb(Soil *fromSoil)
{
    this->soilRef = fromSoil;
    
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    if(roots->size() > 1)
    {
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
            this->climbRecursive((*i)->getRoot(), 0);
    }
    else if(roots->size() == 1)
    {
        this->climbRecursive(roots->front()->getRoot(), 0);
    }
    
    this->soilRef = NULL;
}

void Crow::climbRecursive(NetworkTreeNode *cur, unsigned short depth)
{
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    
    // Puts direct neighbor subnets in a list
    list<NetworkTreeNode*> childrenL;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        // Just in case (PlanetLab...)
        if((*i) == NULL)
            continue;
    
        if((*i)->isLeaf())
            childrenL.push_back((*i));
    }
    size_t nbNeighborSubnets = childrenL.size();
    size_t nbLabels = labels->size();

    if(nbNeighborSubnets > 0 || nbLabels > 0 || (nbLabels == 1 && labels->front() != InetAddress(0)))
    {   
        // Router inference
        ar->setCurrentTTL(depth);
        this->ar->resolve(cur);
        list<Router*> routers = cur->getInferredRouters();
        if((unsigned short) routers.size() > 0)
        {
            for(list<Router*>::iterator i = routers.begin(); i != routers.end(); ++i)
            {
                Router *cur = (*i);
                this->aliases.push_back(cur);
            }
        }
        else
        {
            // No alias resolution could be carried out (lack of data)
        }
    }
    
    // Goes deeper in the tree (avoids exploring leaves)
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i) != NULL && (*i)->isInternal())
            this->climbRecursive((*i), depth + 1);
    }
}

void Crow::outputAliases(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    for(list<Router*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
    {
        newFile << (*i)->toString() << "\n";
    }
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
