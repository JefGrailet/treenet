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

#include "TreeNETEnvironment.h"

Mutex TreeNETEnvironment::consoleMessagesMutex(Mutex::ERROR_CHECKING_MUTEX);
Mutex TreeNETEnvironment::emergencyStopMutex(Mutex::ERROR_CHECKING_MUTEX);

TreeNETEnvironment::TreeNETEnvironment(ostream *cOut, 
                                       bool extLogs, 
                                       bool uM, 
                                       unsigned short protocol, 
                                       bool dP, 
                                       bool useFFID, 
                                       InetAddress &localIP, 
                                       string &probeMsg, 
                                       TimeVal &timeout, 
                                       TimeVal &regulatingPeriod, 
                                       TimeVal &threadDelay, 
                                       unsigned short nIDs, 
                                       unsigned short mRollovers, 
                                       double bTol, 
                                       double mError, 
                                       unsigned short dMode, 
                                       unsigned short mT):
consoleOut(cOut), 
externalLogs(extLogs), 
isExternalLogOpened(false), 
usingMerging(uM), 
probingProtocol(protocol), 
doubleProbe(dP),
useFixedFlowID(useFFID), 
localIPAddress(localIP), 
probeAttentionMessage(probeMsg), 
timeoutPeriod(timeout), 
probeRegulatingPeriod(regulatingPeriod), 
probeThreadDelay(threadDelay), 
nbIPIDs(nIDs), 
maxRollovers(mRollovers), 
baseTolerance(bTol), 
maxError(mError), 
displayMode(dMode), 
maxThreads(mT), 
totalProbes(0), 
totalSuccessfulProbes(0), 
flagEmergencyStop(false)
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
    if(this->externalLogs || this->isExternalLogOpened)
        return &this->logStream;
    return this->consoleOut;
}

void TreeNETEnvironment::copySubnetSet(SubnetSiteSet *src)
{
    list<SubnetSite*> *ssListDst = subnetSet->getSubnetSiteList();
    list<SubnetSite*> *ssListSrc = src->getSubnetSiteList();
    ssListDst->clear();
    for(list<SubnetSite*>::iterator it = ssListSrc->begin(); it != ssListSrc->end(); ++it)
    {
        ssListDst->push_back(*it);
    }
}

void TreeNETEnvironment::transferSubnetSet(SubnetSiteSet *src)
{
    list<SubnetSite*> *ssListDst = subnetSet->getSubnetSiteList();
    list<SubnetSite*> *ssListSrc = src->getSubnetSiteList();
    while(ssListSrc->size() > 0)
    {
        ssListDst->push_back(ssListSrc->front());
        ssListSrc->pop_front();
    }
    subnetSet->sortSet();
}

void TreeNETEnvironment::updateProbeAmounts(DirectProber *proberObject)
{
    totalProbes += proberObject->getNbProbes();
    totalSuccessfulProbes += proberObject->getNbSuccessfulProbes();
}

void TreeNETEnvironment::resetProbeAmounts()
{
    totalProbes = 0;
    totalSuccessfulProbes = 0;
}

void TreeNETEnvironment::openLogStream(string filename, bool message)
{
    this->logStream.open(filename.c_str());
    this->isExternalLogOpened = true;
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    if(message)
        (*consoleOut) << "Log of current phase is being written in " << filename << ".\n" << endl;
}

void TreeNETEnvironment::closeLogStream()
{
    this->logStream.close();
    this->logStream.clear();
    this->isExternalLogOpened = false;
}

bool TreeNETEnvironment::triggerStop()
{
    /*
     * In case of loss of connectivity, it is possible several threads calls this method. To avoid 
     * multiple contexts launching the emergency stop, there is the following condition (in 
     * addition to a Mutex object).
     */
    
    if(flagEmergencyStop)
    {
        return false;
    }

    flagEmergencyStop = true;
    
    consoleMessagesMutex.lock();
    (*consoleOut) << "\nWARNING: emergency stop has been triggered because of connectivity ";
    (*consoleOut) << "issues.\nTreeNET will wait for all remaining threads to complete before ";
    (*consoleOut) << "exiting without carrying out remaining probing tasks.\n" << endl;
    consoleMessagesMutex.unlock();
    
    return true;
}

void TreeNETEnvironment::resetIPDictionnary()
{
    delete IPTable;
    IPTable = new IPLookUpTable(nbIPIDs);
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
                InetAddress curIP = route[i].ip;
                if(curIP == InetAddress(0))
                    continue;
                
                IPTableEntry *entry = IPTable->lookUp(curIP);
                if(entry == NULL)
                {
                    entry = IPTable->create(curIP);
                    entry->setTTL(TTL);
                }
                else if(!entry->sameTTL(TTL) && !entry->hasHopCount(TTL))
                {
                    entry->recordHopCount(TTL);
                }
            }
        }
    }
}
