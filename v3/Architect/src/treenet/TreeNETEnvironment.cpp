/*
 * TreeNETEnvironment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file implements the class defined in TreeNETEnvironment.h. See this file for more details 
 * on the goals of such class.
 */

#include <sys/stat.h> // For CHMOD edition
#include <list>
using std::list;

#include "TreeNETEnvironment.h"

TreeNETEnvironment::TreeNETEnvironment(ostream *cOut, 
                                       bool uM, 
                                       unsigned short nIDs, 
                                       unsigned short mRollovers, 
                                       double bTol, 
                                       double mError, 
                                       unsigned short dMode):
consoleOut(cOut), 
isExternalLogOpened(false), 
usingMerging(uM), 
nbIPIDs(nIDs), 
maxRollovers(mRollovers), 
baseTolerance(bTol), 
maxError(mError), 
displayMode(dMode)
{
    this->IPTable = new IPLookUpTable(nIDs);
    this->subnetSet = new SubnetSiteSet();
}

TreeNETEnvironment::~TreeNETEnvironment()
{
    delete IPTable;
    delete subnetSet;
}

ostream* TreeNETEnvironment::getOutputStream()
{
    if(this->isExternalLogOpened)
        return &this->logStream;
    return this->consoleOut;
}

void TreeNETEnvironment::openLogStream(string filename)
{
    this->logStream.open(filename.c_str());
    this->isExternalLogOpened = true;
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void TreeNETEnvironment::closeLogStream()
{
    this->logStream.close();
    this->logStream.clear();
    this->isExternalLogOpened = false;
}

void TreeNETEnvironment::fillIPDictionnary()
{
    list<SubnetSite*> *subnets = this->subnetSet->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        
        // IPs mentioned as interfaces
        list<SubnetSiteNode*> *IPs = curSubnet->getSubnetIPList();
        for(list<SubnetSiteNode*>::iterator i = IPs->begin(); i != IPs->end(); ++i)
        {
            SubnetSiteNode *curIP = (*i);
            
            IPTableEntry *newEntry = this->IPTable->create(curIP->ip);
            if(newEntry != NULL)
                newEntry->setTTL(curIP->TTL);
        }
        
        // IPs appearing in routes
        unsigned short routeSize = curSubnet->getRouteSize();
        RouteInterface *route = curSubnet->getRoute();
        if(routeSize > 0 && route != NULL)
        {
            for(unsigned short i = 0; i < routeSize; i++)
            {
                unsigned char TTL = (unsigned char) i + 1;
                
                IPTableEntry *newEntry = this->IPTable->create(route[i].ip);
                if(newEntry != NULL)
                    newEntry->setTTL(TTL);
            }
        }
    }
}
