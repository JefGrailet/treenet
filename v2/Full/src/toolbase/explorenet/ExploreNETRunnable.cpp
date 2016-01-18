/*
 * ExploreNETRunnable.cpp
 *
 *  Created on: Oct 04, 2014
 *      Author: grailet
 *
 * This class fuses the classes ExploreNETRunnableBox and ExploreNETRunnableSingleInput from
 * the original ExploreNET (v2.1) into a single thread class. The goal is to redesign this
 * part of ExploreNET in order to embed it in a larger topology discovery tool (as the
 * class ExploreNETRunnableMultipleInput is expected to be useless in this context).
 */

#include "ExploreNETRunnable.h"

// 7 next lines are for print out purpose only
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iomanip>
using std::left;
using std::right;

Mutex ExploreNETRunnable::sssMutex(Mutex::ERROR_CHECKING_MUTEX);
Mutex ExploreNETRunnable::outMutex(Mutex::ERROR_CHECKING_MUTEX);

ExploreNETRunnable::ExploreNETRunnable(TreeNETEnvironment *e, 
                                       InetAddress &t, 
                                       unsigned short lowerBoundSrcPortORICMPid, 
                                       unsigned short upperBoundSrcPortORICMPid, 
                                       unsigned short lowerBoundDstPortORICMPseq, 
                                       unsigned short upperBoundDstPortICMPseq) throw (SocketException):
env(e),
target(t),
sinf(e, 
     lowerBoundSrcPortORICMPid, 
     upperBoundSrcPortORICMPid, 
     lowerBoundDstPortORICMPseq, 
     upperBoundDstPortICMPseq)
{
}

ExploreNETRunnable::~ExploreNETRunnable() {}

/**
 * Usual run method.
 */

void ExploreNETRunnable::run()
{
    InetAddress t;
    SubnetSite *site = NULL;
    unsigned short res;
    
    // Parameters obtained through env
    IPLookUpTable *table = env->getIPTable();
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    NetworkAddress LAN = env->getLAN();
    unsigned char startTTL = env->getStartTTL();
    bool exploreLANExplicitly = env->exploringLANExplicitly();
    bool useLowerBorderAsWell = env->usingLowerBorderAsWell();

    t = this->target;
    if(t.isUnset())
    {
        return;
    }
    
    // Checks that target is not already covered in the set
    sssMutex.lock();
    SubnetSite *coverage = subnetSet->getSubnetContaining(t);
    if(coverage != NULL)
    {
        outMutex.lock();
        cout << t << " is already in previously inferred subnet ";
        cout << coverage->getInferredNetworkAddressString() << endl;
        
        // Finds and gives TTL to that target in IP table
        list<SubnetSiteNode*> *nodes = coverage->getSubnetIPList();
        unsigned char minTTL = 255;
        unsigned char tTTL = 0;
        for(std::list<SubnetSiteNode*>::iterator it = nodes->begin(); it != nodes->end(); ++it)
        {
            if((*it)->ip == t)
            {
                tTTL = (*it)->TTL;
                break;
            }
            
            if((*it)->TTL < minTTL)
                minTTL = (*it)->TTL;
        }
        
        IPTableEntry *entry = table->lookUp(t);
        if(entry != NULL) // Just in case
        {
            if(tTTL != 0)
                entry->setTTL(tTTL);
            else
                entry->setTTL(minTTL + 1); // Pivot TTL
        }
        
        outMutex.unlock();
        sssMutex.unlock();
        return;
    }
    sssMutex.unlock();
    
    try
    {
        // Infers local subnet if the target is within the range of the LAN
        if(LAN.subsumes(t))
        {
            // Infers local subnet with all IPs or use local information
            if(exploreLANExplicitly == true)
            {
                site = sinf.inferLocalAreaSubnet(t, LAN);
                res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE;
            }
            else
            {
                site = sinf.inferDummyLocalAreaSubnet(t, LAN);
                res = ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE;
            }
        }
        // Infers remote subnet
        else
        {
            site = sinf.inferRemoteSubnet(t, false, startTTL, useLowerBorderAsWell);
            if(site != 0 && site->getInferredSubnetPrefixLength() <= 32)
            {
                if(site->getRefinementStatus() == SubnetSite::NOT_PREPARED_YET)
                {
                    res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE;
                }
                else
                {
                    res = ExploreNETRunnable::UNNECESSARY_PROBING;
                }
            }
            else
            {
                res = ExploreNETRunnable::NULL_SUBNET_SITE;
            }
        }
    }
    catch (UnresponsiveIPException &e)
    {
        res = ExploreNETRunnable::UNRESPONSIVE_IP_EXCEPTION;
    }
    catch (UndesignatedPivotInterface &e)
    {
        res = ExploreNETRunnable::UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION;
    }
    catch (ShortTTLException &e)
    {
        res = ExploreNETRunnable::SHORT_TTL_EXCEPTION;
    }
    
    string stringResult = ""; // For output purpose
    
    // Registering the site in the set
    if(res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE ||
       res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE ||
       res == ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE)
    {
        sssMutex.lock();
        unsigned short insertionResult = subnetSet->addSite(site);
        string networkAddressStr = site->getInferredNetworkAddressString();
        
        if(insertionResult == SubnetSiteSet::SMALLER_SUBNET || 
           insertionResult == SubnetSiteSet::KNOWN_SUBNET)
        {
            delete site;
            stringResult += "inferred " + networkAddressStr + ", merged with larger/equivalent subnet";
        }
        else
        {
            stringResult += "inferred " + networkAddressStr + ", new subnet";
        }
        
        site = 0;
        sssMutex.unlock();
    }
    // Failure: display cause of the problem in console if showInferenceFailures is true
    else
    {
        /*
         * TreeNET v2.1: no inference for IP encompassed by a /20 block with the same TTL for 
         * Pivot IPs, because previous inference/expansion led to subnet(s) with no Contra-Pivot 
         * to ensure their soundness.
         */
        
        if(res == ExploreNETRunnable::UNNECESSARY_PROBING)
        {
            stringResult += "no inference, as pivot TTL is the same as for IPs in ";
            stringResult += site->getInferredNetworkAddressString();
            stringResult += " (IP range to avoid)";
            
            // Avoids deleting the /20 block afterwards
            site = 0;
        }
        // Other errors already present in TreeNET v2.0
        else if(res == ExploreNETRunnable::NULL_SUBNET_SITE)
            stringResult += "subnet seems to not exist";
        else if(res == ExploreNETRunnable::SHORT_TTL_EXCEPTION)
            stringResult += "subnet TTL (pivot TTL) must be at least two";
        else if(res == ExploreNETRunnable::UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION)
            stringResult += "undesignated pivot IP address";
        else if(res == ExploreNETRunnable::UNRESPONSIVE_IP_EXCEPTION)
            stringResult += "unresponsive target IP address";
        else
            stringResult += "error in the code; uninitialized/unknown error type";
    }

    // Delete the site object if initialized
    if(site != 0)
    {
        delete site;
        site = 0;
    }
    
    outMutex.lock();
    cout << "Probed " << t << ": " << stringResult << endl;
    outMutex.unlock();
    
    return;
}
