/*
 * TreeNETEnvironment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: grailet
 *
 * This file implements the class defined in TreeNETEnvironment.h. See this file for more details 
 * on the goals of such class.
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include <iostream>
using std::ostream;
#include <cmath> // For floor()

#include "TreeNETEnvironment.h"

using namespace std;

TreeNETEnvironment::TreeNETEnvironment(ostream *o, 
                                       unsigned short protocol, 
                                       bool dP, 
                                       bool useFFID, 
                                       InetAddress &localIP, 
                                       NetworkAddress &lan, 
                                       string &probeMsg, 
                                       TimeVal &timeout, 
                                       TimeVal &regulatingPeriod, 
                                       TimeVal &threadDelay, 
                                       unsigned short nIDs, 
                                       unsigned short mRollovers, 
                                       double bTol, 
                                       double mError, 
                                       bool dbg, 
                                       unsigned short mT):
out(o), 
probingProtocol(protocol), 
doubleProbe(dP),
useFixedFlowID(useFFID), 
localIPAddress(localIP), 
LAN(lan), 
probeAttentionMessage(probeMsg), 
timeoutPeriod(timeout), 
probeRegulatingPeriod(regulatingPeriod), 
probeThreadDelay(threadDelay), 
nbIPIDs(nIDs), 
maxRollovers(mRollovers), 
baseTolerance(bTol), 
maxError(mError), 
debug(dbg), 
maxThreads(mT)
{
    this->IPTable = new IPLookUpTable(nIDs);
}

TreeNETEnvironment::~TreeNETEnvironment()
{
    delete IPTable;

    for(list<Router*>::iterator i = routerList.begin(); i != routerList.end(); ++i)
        delete (*i);
    
    routerList.clear();
}

void TreeNETEnvironment::outputRouterList(string fileName)
{
    string output = "";
    for(list<Router*>::iterator i = routerList.begin(); i != routerList.end(); ++i)
    {
        Router *r = (*i);
        string cur = r->toString();
        
        if(!cur.empty())
            output += cur;
    }
    
    ofstream newFile;
    newFile.open(fileName.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + fileName;
    chmod(path.c_str(), 0766);
}

void TreeNETEnvironment::outputPlotData(string fileName)
{
    list<double> ratios;
    for(list<Router*>::iterator i = routerList.begin(); i != routerList.end(); ++i)
    {
        if((*i)->canBeInferred())
            ratios.push_back((*i)->getSuccessRatio() / 100);
        else
            ratios.push_back(-1.0);
    }
    ratios.sort();

    stringstream output;
    for(list<double>::iterator i = ratios.begin(); i != ratios.end(); ++i)
        output << (*i) << "\n";
    
    ofstream newFile;
    newFile.open(fileName.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + fileName;
    chmod(path.c_str(), 0766);
}
