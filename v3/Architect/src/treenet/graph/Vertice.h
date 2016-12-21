/*
 * Vertice.h
 *
 *  Created on: Nov 25, 2016
 *      Author: jefgrailet
 *
 * Vertice is a superclass used to model the different types of vertices used in the graphs 
 * produced by TreeNET "Architect".
 */

#ifndef VERTICE_H_
#define VERTICE_H_

#include <list>
using std::list;
#include <string>
using std::string;

// Inclusion of Edge with forward declaration
#include "Edge.h"
class Edge;

class Vertice
{
public:

    Vertice(unsigned int ID);
    virtual ~Vertice();
    
    Vertice(Vertice *toCopy);
    
    inline unsigned int getID() { return ID; }
    inline list<Edge*> *getIncidentEdges() { return &incidentEdges; }
    
    // Methods to handle connections to this vertice
    inline void addIncidentEdge(Edge *e) { incidentEdges.push_back(e); }
    inline unsigned int getDegree() { return incidentEdges.size(); }
    bool isConnectedTo(Vertice *v);
    
    inline static bool smaller(Vertice *v1, Vertice *v2) { return (v1->getID() < v2->getID()); }
    
    /*
     * Virtual toString() methods, to be implemented by children classes; one just gives the 
     * label (e.g. for a subnet, it will return S[ID]), the second should provide a bit more 
     * details (e.g. for a subnet denoted S[ID], it also writes beside the network address 
     * associated to it).
     */
    
    virtual string toString() = 0;
    virtual string toStringDetailed() = 0;

protected:

    unsigned int ID;
    list<Edge*> incidentEdges;
    
};

#endif /* VERTICE_H_ */
