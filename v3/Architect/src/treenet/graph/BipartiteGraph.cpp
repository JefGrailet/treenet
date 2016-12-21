/*
 * BipartiteGraph.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in BipartiteGraph.h (see this file to learn further about the 
 * goals of such class).
 */

#include <sstream>
using std::stringstream;

#include "BipartiteGraph.h"

BipartiteGraph::BipartiteGraph() : Graph()
{
    counterPartyOne = 1;
    counterPartyTwo = 1;
}

BipartiteGraph::~BipartiteGraph()
{
    for(list<Vertice*>::iterator it = partyOne.begin(); it != partyOne.end(); ++it)
    {
        delete (*it);
    }
    partyOne.clear();
    
    for(list<Vertice*>::iterator it = partyTwo.begin(); it != partyTwo.end(); ++it)
    {
        delete (*it);
    }
    partyTwo.clear();
}

string BipartiteGraph::partyOneToString()
{
    stringstream ss;
    for(list<Vertice*>::iterator it = partyOne.begin(); it != partyOne.end(); ++it)
    {
        ss << (*it)->toStringDetailed() << "\n";
    }
    return ss.str();
}

string BipartiteGraph::partyTwoToString()
{
    stringstream ss;
    for(list<Vertice*>::iterator it = partyTwo.begin(); it != partyTwo.end(); ++it)
    {
        ss << (*it)->toStringDetailed() << "\n";
    }
    return ss.str();
}

string BipartiteGraph::getMetricsString(bool showUnconnected)
{
    unsigned int nbVerticesLeft = partyOne.size();
    unsigned int minDegreeL = 0, maxDegreeL = 0, sumDegreesL = 0;
    unsigned int nbVerticesRight = partyTwo.size();
    unsigned int minDegreeR = 0, maxDegreeR = 0, sumDegreesR = 0;
    
    // Evaluates degree of left party
    for(list<Vertice*>::iterator it = partyOne.begin(); it != partyOne.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        sumDegreesL += curDegree;
        
        if(curDegree > maxDegreeL)
            maxDegreeL = curDegree;
        else if(minDegreeL == 0 || curDegree < minDegreeL)
            minDegreeL = curDegree;
    }
    
    // Does the same for the right party
    for(list<Vertice*>::iterator it = partyTwo.begin(); it != partyTwo.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        sumDegreesR += curDegree;
        
        if(curDegree > maxDegreeR)
            maxDegreeR = curDegree;
        else if(minDegreeR == 0 || curDegree < minDegreeR)
            minDegreeR = curDegree;
    }
    
    float avgDegreeL = (float) sumDegreesL / (float) nbVerticesLeft;
    float avgDegreeR = (float) sumDegreesR / (float) nbVerticesRight;
    float density = (float) edges.size() / (float) (partyOne.size() * partyTwo.size());
    
    // Second pass to list the vertices bearing max degree and the amount for min degree
    list<string> maxDegreeLabelsL;
    list<string> unconnectedL;
    unsigned int nbMinDegreeL = 0;
    for(list<Vertice*>::iterator it = partyOne.begin(); it != partyOne.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        
        if(curDegree == 0)
            unconnectedL.push_back((*it)->toString());
        
        if(curDegree == maxDegreeL)
            maxDegreeLabelsL.push_back((*it)->toString());
        else if(curDegree == minDegreeL)
            nbMinDegreeL++;
    }
    
    list<string> maxDegreeLabelsR;
    list<string> unconnectedR;
    unsigned int nbMinDegreeR = 0;
    for(list<Vertice*>::iterator it = partyTwo.begin(); it != partyTwo.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        
        if(curDegree == 0)
            unconnectedR.push_back((*it)->toString());
        
        if(curDegree == maxDegreeR)
            maxDegreeLabelsR.push_back((*it)->toString());
        else if(curDegree == minDegreeR)
            nbMinDegreeR++;
    }
    
    // Starts writing everything into a string stream
    stringstream ss;
    ss << "Minimum degree:\n";
    ss << "-left party: " << minDegreeL << " (" << nbMinDegreeL << " vertices)\n";
    ss << "-right party: " << minDegreeR << " (" << nbMinDegreeR << " vertices)\n";
    ss << "Maximum degree:\n";
    ss << "-left party: " << maxDegreeL << " (";
    bool first = true;
    for(list<string>::iterator it = maxDegreeLabelsL.begin(); it != maxDegreeLabelsL.end(); ++it)
    {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << (*it);
    }
    ss << ")\n";
    ss << "-right party: " << maxDegreeR << " (";
    first = true;
    for(list<string>::iterator it = maxDegreeLabelsR.begin(); it != maxDegreeLabelsR.end(); ++it)
    {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << (*it);
    }
    ss << ")\n";
    ss << "Average degree:\n";
    ss << "-left party: " << avgDegreeL << "\n";
    ss << "-right party: " << avgDegreeR << "\n";
    ss << "Density: " << density << "\n";
    
    if(showUnconnected && unconnectedL.size() > 0)
    {
        ss << "Unconnected vertices (left party): ";
        first = true;
        for(list<string>::iterator it = unconnectedL.begin(); it != unconnectedL.end(); ++it)
        {
            if(first)
                first = false;
            else
                ss << ", ";
            ss << (*it);
        }
        ss << "\n";
    }
    
    if(showUnconnected && unconnectedR.size() > 0)
    {
        ss << "Unconnected vertices (right party): ";
        first = true;
        for(list<string>::iterator it = unconnectedR.begin(); it != unconnectedR.end(); ++it)
        {
            if(first)
                first = false;
            else
                ss << ", ";
            ss << (*it);
        }
        ss << "\n";
    }
    
    return ss.str();
}
