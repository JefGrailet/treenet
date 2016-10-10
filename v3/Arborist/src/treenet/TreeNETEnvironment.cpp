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
using std::ofstream;
#include <iomanip> // For header lines in the output file displaying ExploreNET records
using std::left;
using std::setw;

#include "TreeNETEnvironment.h"

Mutex TreeNETEnvironment::consoleMessagesMutex(Mutex::ERROR_CHECKING_MUTEX);

TreeNETEnvironment::TreeNETEnvironment(ostream *o, 
                                       unsigned char sTTL, 
                                       unsigned short protocol, 
                                       bool exploreLAN, 
                                       bool useLowerB, 
                                       bool dP, 
                                       bool useFFID, 
                                       bool prescanExp, 
                                       bool saveXnetRec, 
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
                                       unsigned short dMode, 
                                       unsigned short mT):
out(o), 
startTTL(sTTL), 
probingProtocol(protocol), 
exploreLANExplicitly(exploreLAN), 
useLowerBorderAsWell(useLowerB), 
doubleProbe(dP),
useFixedFlowID(useFFID), 
prescanExpand(prescanExp), 
saveExploreNETRecords(saveXnetRec), 
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
displayMode(dMode), 
maxThreads(mT)
{
    this->IPTable = new IPLookUpTable(nIDs);
    this->subnetSet = new SubnetSiteSet();
    this->IPBlocksToAvoid = new SubnetSiteSet();
}

TreeNETEnvironment::~TreeNETEnvironment()
{
    delete IPTable;
    delete subnetSet;
    delete IPBlocksToAvoid;
    
    for(list<ExploreNETRecord*>::iterator it = xnetRecords.begin(); it != xnetRecords.end(); it++)
    {
        delete (*it);
    }
}

void TreeNETEnvironment::outputExploreNETRecords(string filename)
{
    xnetRecords.sort(ExploreNETRecord::compare);

    ofstream newFile;
    newFile.open(filename.c_str());
    
    // Header lines
    newFile << left << setw(20) << "Target IP";
    newFile << left << setw(25) << "Inferred subnet";
    newFile << left << setw(6) << "SPC";
    newFile << left << setw(6) << "SIC";
    newFile << left << setw(18) << "Alternative";
    newFile << "\n";
    
    newFile << left << setw(20) << "---------";
    newFile << left << setw(25) << "---------------";
    newFile << left << setw(6) << "---";
    newFile << left << setw(6) << "---";
    newFile << left << setw(18) << "-----------";
    newFile << "\n";
    
    for(list<ExploreNETRecord*>::iterator it = xnetRecords.begin(); it != xnetRecords.end(); it++)
    {
        newFile << (*it)->toString() << "\n";
    }
    
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
