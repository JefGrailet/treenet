/*
 * ExploreNETEnvironment.cpp
 *
 *  Created on: Nov 2, 2015
 *      Author: grailet
 *
 * This file implements the class defined in ExploeNETEnvironment.h. See this file for more 
 * details on the goals of such class.
 */

#include "ExploreNETEnvironment.h"

ExploreNETEnvironment::ExploreNETEnvironment(ostream *o, 
                                             unsigned char sTTL, 
                                             unsigned short protocol, 
                                             bool exploreLAN, 
                                             bool useLowerB, 
                                             bool dP, 
                                             bool useFFID, 
                                             InetAddress &localIP, 
                                             NetworkAddress &lan, 
                                             string &probeMsg, 
                                             TimeVal &timeout, 
                                             TimeVal &regulatingPeriod, 
                                             TimeVal &threadDelay, 
                                             bool dbg, 
                                             unsigned short mT):
out(o), 
startTTL(sTTL), 
probingProtocol(protocol), 
exploreLANExplicitly(exploreLAN), 
useLowerBorderAsWell(useLowerB), 
doubleProbe(dP),
useFixedFlowID(useFFID), 
localIPAddress(localIP), 
LAN(lan), 
probeAttentionMessage(probeMsg), 
timeoutPeriod(timeout), 
probeRegulatingPeriod(regulatingPeriod), 
probeThreadDelay(threadDelay), 
debug(dbg), 
maxThreads(mT)
{
    this->subnetSet = new SubnetSiteSet();
}

ExploreNETEnvironment::~ExploreNETEnvironment()
{
    delete subnetSet;
}
