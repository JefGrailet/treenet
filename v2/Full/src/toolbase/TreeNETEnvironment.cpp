/*
 * TreeNETEnvironment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: grailet
 *
 * This file implements the class defined in TreeNETEnvironment.h. See this file for more details 
 * on the goals of such class.
 */

#include "TreeNETEnvironment.h"

TreeNETEnvironment::TreeNETEnvironment(ostream *o, 
                                       unsigned char sTTL, 
                                       unsigned short protocol, 
                                       bool exploreLAN, 
                                       bool useLowerB, 
                                       bool dP, 
                                       bool useFFID, 
                                       bool prescanExp, 
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
startTTL(sTTL), 
probingProtocol(protocol), 
exploreLANExplicitly(exploreLAN), 
useLowerBorderAsWell(useLowerB), 
doubleProbe(dP),
useFixedFlowID(useFFID), 
prescanExpand(prescanExp), 
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
    this->subnetSet = new SubnetSiteSet();
}

TreeNETEnvironment::~TreeNETEnvironment()
{
    delete IPTable;
    delete subnetSet;
}
