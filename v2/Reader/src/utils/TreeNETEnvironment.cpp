/*
 * TreeNETEnvironment.cpp
 *
 *  Created on: Jan 5, 2016
 *      Author: grailet
 *
 * This file implements the class defined in TreeNETEnvironment.h. See this file for more details 
 * on the goals of such class.
 */

#include "TreeNETEnvironment.h"

// Constants
TimeVal TreeNETEnvironment::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD(0, 100000);
TimeVal TreeNETEnvironment::DEFAULT_PROBE_TIMEOUT_PERIOD(2, 0);
bool TreeNETEnvironment::DEFAULT_DEBUG = false;
unsigned short TreeNETEnvironment::ALIAS_RESO_DEFAULT_NB_IP_IDS = 4;
unsigned short TreeNETEnvironment::ALIAS_RESO_DEFAULT_MAX_ROLLOVERS = 5;
double TreeNETEnvironment::ALIAS_RESO_DEFAULT_BASE_TOLERANCE = 5.0;
double TreeNETEnvironment::ALIAS_RESO_DEFAULT_MAX_ERROR = 0.3;

TreeNETEnvironment::TreeNETEnvironment(ostream *o, 
                                       bool sR, 
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
                                       bool dbg, 
                                       unsigned short mT):
out(o), 
setRefinement(sR), 
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
debug(dbg), 
maxThreads(mT)
{
    this->IPTable = new IPLookUpTable(nIDs);
    this->subnetSet = new SubnetSiteSet();
}

TreeNETEnvironment::~TreeNETEnvironment()
{
    delete IPTable;
    delete subnetSet;
}
