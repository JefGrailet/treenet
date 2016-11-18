/*
 * TreeNETEnvironment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file implements the class defined in TreeNETEnvironment.h. See this file for more details 
 * on the goals of such class.
 */

#include "TreeNETEnvironment.h"

Mutex TreeNETEnvironment::consoleMessagesMutex(Mutex::ERROR_CHECKING_MUTEX);
Mutex TreeNETEnvironment::emergencyStopMutex(Mutex::ERROR_CHECKING_MUTEX);

TreeNETEnvironment::TreeNETEnvironment(ostream *o, 
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
out(o), 
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
    (*out) << "\nWARNING: emergency stop has been triggered because of connectivity issues.\n";
    (*out) << "TreeNET will wait for all remaining threads to complete before exiting without ";
    (*out) << "carrying out remaining probing tasks.\n" << endl;
    consoleMessagesMutex.unlock();
    
    return true;
}
