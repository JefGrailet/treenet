/*
 * SimpleGraph.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in SimpleGraph.h (see this file to learn further about the goals 
 * of such class).
 */

#include <sstream>
using std::stringstream;

#include "SimpleGraph.h"

SimpleGraph::SimpleGraph()
{
}

SimpleGraph::~SimpleGraph()
{
    for(list<Vertice*>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    {
        delete (*it);
    }
    vertices.clear();
}

string SimpleGraph::verticesToString()
{
    stringstream ss;
    for(list<Vertice*>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    {
        ss << (*it)->toStringDetailed() << "\n";
    }
    return ss.str();
}

string SimpleGraph::getMetricsString()
{
    unsigned int nbVertices = vertices.size();
    unsigned int minDegree = 0, maxDegree = 0, sumDegrees = 0;
    
    // Evaluates degree
    for(list<Vertice*>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        sumDegrees += curDegree;
        
        if(curDegree > maxDegree)
            maxDegree = curDegree;
        else if(minDegree == 0 || curDegree < minDegree)
            minDegree = curDegree;
    }
    
    float avgDegree = (float) sumDegrees / (float) nbVertices;
    float density = (float) (2 * edges.size()) / (float) (nbVertices * (nbVertices - 1));
    
    // Second pass to list the vertices bearing max degree and the amount for min degree
    list<string> maxDegreeLabels;
    list<string> unconnected;
    unsigned int nbMinDegree = 0;
    for(list<Vertice*>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    {
        unsigned int curDegree = (*it)->getDegree();
        
        if(curDegree == 0)
            unconnected.push_back((*it)->toString());
        
        if(curDegree == maxDegree)
            maxDegreeLabels.push_back((*it)->toString());
        else if(curDegree == minDegree)
            nbMinDegree++;
    }
    
    // Starts writing everything into a string stream
    stringstream ss;
    ss << "Minimum degree: " << minDegree << " (" << nbMinDegree << " vertices)\n";
    ss << "Maximum degree: " << maxDegree << " (";
    bool first = true;
    for(list<string>::iterator it = maxDegreeLabels.begin(); it != maxDegreeLabels.end(); ++it)
    {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << (*it);
    }
    ss << ")\n";
    ss << "Average degree: " << avgDegree << "\n";
    ss << "Density: " << density << "\n";
    if(unconnected.size() > 0)
    {
        ss << "Unconnected vertices: ";
        first = true;
        for(list<string>::iterator it = unconnected.begin(); it != unconnected.end(); ++it)
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
