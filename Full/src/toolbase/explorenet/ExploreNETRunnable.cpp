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

ExploreNETRunnable::ExploreNETRunnable(SubnetSiteSet *sss, 
                                       TargetAddress &t, 
                                       InetAddress &localIPAddress, 
                                       NetworkAddress &l, 
                                       bool expLAN, 
                                       bool ulbaw, 
                                       string &probeAttentionMessage, 
                                       TimeVal &timeoutPeriod, 
                                       TimeVal &probeRegulatingPeriod, 
                                       bool doubleProbe, 
                                       bool useFixedFlowID, 
                                       unsigned short lowerBoundSrcPortORICMPid, 
                                       unsigned short upperBoundSrcPortORICMPid, 
                                       unsigned short lowerBoundDstPortORICMPseq, 
                                       unsigned short upperBoundDstPortICMPseq, 
                                       bool dbg) throw (SocketException):
set(sss),
target(t),
useLowerBorderAsWell(ulbaw),
lan(l),
exploreLANexplicitly(expLAN),
sinf(localIPAddress, 
     probeAttentionMessage, 
     timeoutPeriod, 
     probeRegulatingPeriod, 
     doubleProbe, 
     useFixedFlowID, 
     lowerBoundSrcPortORICMPid, 
     upperBoundSrcPortORICMPid, 
     lowerBoundDstPortORICMPseq, 
     upperBoundDstPortICMPseq,
     dbg)
{
}

ExploreNETRunnable::~ExploreNETRunnable() {}

/**
 * Usual run method.
 */

void ExploreNETRunnable::run()
{
	TargetAddress t;
	SubnetSite *site = NULL;
	unsigned short res;

	t = this->target;
	if(t.address.isUnset())
	{
		return;
	}
	
	// Checks that target is not already covered in the set
	sssMutex.lock();
	if(set->isCovered(t.address))
	{
	    outMutex.lock();
	    cout << t.address << " is already in previously inferred subnets" << endl;
	    outMutex.unlock();
	    sssMutex.unlock();
	    return;
	}
	sssMutex.unlock();
	
	try
	{
	    // Infers local subnet if the target is within the range of the LAN
		if(lan.subsumes(t.address))
		{
		    // Infers local subnet with all IPs or use local information
			if(exploreLANexplicitly == true)
			{
				site = sinf.inferLocalAreaSubnet(t.address, lan);
				res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE;
			}
			else
			{
				site = sinf.inferDummyLocalAreaSubnet(t.address, lan);
				res = ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE;
			}
		}
		// Infers remote subnet
		else
		{
			site = sinf.inferRemoteSubnet(t.address, false, t.startTTL, useLowerBorderAsWell);
			if(site != 0 && site->getInferredSubnetPrefixLength() <= 32)
			{
				res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE;
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
	    unsigned short insertionResult = set->addSite(site);
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
        if(res == ExploreNETRunnable::NULL_SUBNET_SITE)
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
	cout << "Probed " << t.address << ": " << stringResult << endl;
	outMutex.unlock();
	
	return;
}

