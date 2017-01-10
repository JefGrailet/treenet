/*
 * NSProcesser.cpp
 *
 *  Created on: Nov 29, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in NSProcesser.h (see this file to learn further about the goals 
 * of such class).
 */

#include <fstream>
using std::ofstream;
#include <sstream>
using std::stringstream;
#include <sys/stat.h> // For CHMOD edition

#include "NSProcesser.h"
#include "proj-n/NeighborhoodGraph.h"
#include "proj-s/SubnetGraph1.h"

NSProcesser::NSProcesser(TreeNETEnvironment *env)
{
    this->env = env;
    this->soilRef = NULL;
    this->result = NULL;
    this->nNeighborhoods = 0;
    this->nSubnets = 0;
    this->checkArrN = NULL;
    this->checkArrS = NULL;
}

NSProcesser::~NSProcesser()
{
}

void NSProcesser::process(Soil *fromSoil)
{
    this->soilRef = fromSoil;
    this->result = new NSGraph();
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    
    if(roots->size() > 1)
    {
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            this->skipTrunkAndStart((*i));
        }
    }
    else if(roots->size() == 1)
    {
        this->skipTrunkAndStart(roots->front());
    }
    
    this->result->sortEdges();
    this->soilRef = NULL;
}

void NSProcesser::output(string filename)
{
    if(this->result == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to output as \"" << filename << "\"." << endl;
        return;
    }
    
    stringstream output;
    output << this->result->partyOneToString() << "\n";
    output << this->result->partyTwoToString() << "\n";
    output << this->result->edgesToString() << "\n";
    output << this->result->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Cleans produced graph
    delete this->result;
    this->result = NULL;
}

double NSProcesser::check()
{
    if(this->result == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error (check() method): no graph to check." << endl;
        return 0.0;
    }

    list<Vertice*> *neighborhoods = this->result->getPartyOne();
    list<Vertice*> *subnets = this->result->getPartyTwo();
    
    if(neighborhoods->size() == 0 || subnets->size() == 0)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error (check() method): one of the parties is empty." << endl;
        return 0.0;
    }
    
    // Sets the arrays (and their size) used for checking
    nNeighborhoods = neighborhoods->size();
    checkArrN = new bool[nNeighborhoods];
    for(unsigned int i = 0; i < nNeighborhoods; i++)
        checkArrN[i] = false;
    
    nSubnets = subnets->size();
    checkArrS = new bool[nSubnets];
    for(unsigned int i = 0; i < nSubnets; i++)
        checkArrS[i] = false;
    
    // Visits the graph
    visit(neighborhoods->front());
    
    unsigned int visitedNeighborhoods = 0;
    for(unsigned int i = 0; i < nNeighborhoods; i++)
        if(checkArrN[i])
            visitedNeighborhoods++;
    
    unsigned int visitedSubnets = 0;
    for(unsigned int i = 0; i < nSubnets; i++)
        if(checkArrS[i])
            visitedSubnets++;
    
    // Frees the arrays
    delete[] checkArrN;
    delete[] checkArrS;
    checkArrN = NULL;
    checkArrS = NULL;
    
    // Computes final result
    double res = (((double) (visitedNeighborhoods + visitedSubnets)) / ((double) (nNeighborhoods + nSubnets))) * 100;
    nNeighborhoods = 0;
    nSubnets = 0;
    return res;
}

void NSProcesser::outputSubnetProjection(string filename)
{
    if(this->result == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to project on subnets in an output file ";
        (*out) << "\"" << filename << "\"." << endl;
        return;
    }
    
    SubnetGraph1 *projection = new SubnetGraph1(this->result);
    
    stringstream output;
    output << projection->verticesToString() << "\n";
    output << projection->edgesToString() << "\n";
    output << projection->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Deletes the projection
    delete projection;
}

void NSProcesser::outputNeighborhoodProjection(string filename)
{
    if(this->result == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to project on neighborhoods in an output file ";
        (*out) << "\"" << filename << "\"." << endl;
        return;
    }
    
    NeighborhoodGraph *projection = new NeighborhoodGraph(this->result);
    
    stringstream output;
    output << projection->verticesToString() << "\n";
    output << projection->edgesToString() << "\n";
    output << projection->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Deletes the projection
    delete projection;
}

void NSProcesser::skipTrunkAndStart(NetworkTree *tree)
{
    // Gets to the first internal node with more than one child.
    NetworkTreeNode *entryPoint = tree->getRoot();
    while(entryPoint->getChildren()->size() == 1)
        entryPoint = entryPoint->getChildren()->front();
    
    // If the node is an Hedera, we will start from the first parent which is not one.
    while(entryPoint->isHedera())
        entryPoint = entryPoint->getParent();
    
    this->processRecursive(entryPoint);
}

void NSProcesser::processRecursive(NetworkTreeNode *cur)
{
    NSGraph *bipGraph = this->result;
    list<NetworkTreeNode*> *children = cur->getChildren();
    
    // Puts direct neighbor subnets in a list and puts the T_NEIGHBORHOOD nodes in another one
    list<SubnetSite*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->isLeaf())
            childrenL.push_back((*i)->getAssociatedSubnet());
        else if((*i)->isInternal())
            childrenI.push_back((*i));
    }
    
    // Goes deeper in the tree first
    for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
    {
        if(!(*i)->isLeaf())
            this->processRecursive((*i));
    }

    // Connects all children subnets to this internal node (= neighborhood)
    if(childrenL.size() > 0)
    {
        for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
            bipGraph->createEdge(cur, (*i));
    }
        
    // Connects all children internal node (= neighborhoods) with this one
    if(childrenI.size() > 0)
    {
        for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
        {
            NetworkTreeNode *curInternal = (*i);
            
            // Just in case
            if(curInternal == NULL)
                continue;
            
            list<InetAddress> *labels = curInternal->getLabels();
            
            /*
             * Tries to find the subnet being crossed to reach this internal node and connects them 
             * together: this will create the path from "cur" internal node to the child internal node.
             */
            
            bool successfullyConnected = false;
            for(list<SubnetSite*>::iterator j = childrenL.begin(); j != childrenL.end(); ++j)
            {
                SubnetSite *curSubnet = (*j);
                
                // Just in case
                if(curSubnet == NULL)
                    continue;
                
                // Checks via the labels (much like in the climber class "Cat")
                for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                {
                    if(curSubnet->contains((*k)))
                    {
                        bipGraph->createEdge(curInternal, curSubnet);
                        successfullyConnected = true;
                        break;
                    }
                }
                
                if(successfullyConnected)
                    break;
            }
            
            /*
             * If there is no crossed subnet, an imaginary subnet or a subnet found elsewhere in 
             * the tree (due to route stretching) is required.
             */
            
            if(!successfullyConnected)
            {
                SubnetSite *connectingSubnet = NULL;
                for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                {
                    SubnetMapEntry *sme = this->soilRef->getSubnetContaining((*k));
                    if(sme != NULL)
                    {
                        connectingSubnet = sme->subnet;
                        break;
                    }
                }
                
                /*
                 * The connecting subnet should NOT be a child node of the current child internal 
                 * node to use it (this would be incoherent regarding Paris Traceroute). In such a 
                 * case, we will prefer an artificial link between both internal nodes.
                 */
                
                bool notAChildSubnet = true;
                if(connectingSubnet != NULL)
                {
                    list<NetworkTreeNode*> *children = curInternal->getChildren();
                    for(list<NetworkTreeNode*>::iterator j = children->begin(); j != children->end(); ++j)
                    {
                        if((*j)->isLeaf() && (*j)->getAssociatedSubnet() == connectingSubnet)
                        {
                            notAChildSubnet = false;
                            break;
                        }
                    }
                }
                
                // If a connecting subnet exists, we use it as a connection medium
                if(connectingSubnet != NULL && notAChildSubnet)
                {
                    bipGraph->createEdge(cur, connectingSubnet);
                    bipGraph->createEdge(curInternal, connectingSubnet);
                }
                // Otherwise, we create an artificial path (i.e., with an imaginary subnet)
                else
                {
                    bipGraph->createArtificialPath(cur, curInternal);
                }
            }
        }
    }
}

void NSProcesser::visit(Vertice *node)
{
    if(Neighborhood *n = dynamic_cast<Neighborhood*>(node))
    {
        unsigned int ID = n->getID();
        if(!checkArrN[ID - 1])
        {
            checkArrN[ID - 1] = true;
            list<Edge*> *edges = n->getIncidentEdges();
            for(list<Edge*>::iterator it = edges->begin(); it != edges->end(); ++it)
            {
                visit((*it)->getVerticeTwo()); // The subnet is always the second vertice
            }
        }
    }
    else if(Subnet *s = dynamic_cast<Subnet*>(node))
    {
        unsigned int ID = s->getID();
        if(!checkArrS[ID - 1])
        {
            checkArrS[ID - 1] = true;
            list<Edge*> *edges = s->getIncidentEdges();
            for(list<Edge*>::iterator it = edges->begin(); it != edges->end(); ++it)
            {
                visit((*it)->getVerticeOne()); // The neighborhood is always the first vertice
            }
        }
    }
}
