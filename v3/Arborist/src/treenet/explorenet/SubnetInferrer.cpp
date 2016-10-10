/*
 * SubnetInferrer.cpp
 *
 *  Created on: Jul 15, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (october 2014) to improve coding style and study the code
 */

#include "SubnetInferrer.h"
#include <set>
using std::set;
#include <sstream>
using std::stringstream;
#include <math.h>

#include "../../common/thread/Thread.h"
#include "../../common/inet/NetworkAddress.h"
#include "../../common/exception/NTmapException.h"

TimeVal SubnetInferrer::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD(0, 100000);
unsigned short SubnetInferrer::DEFAULT_MAX_CONSECUTIVE_ANONYMOUS = 4;
unsigned short SubnetInferrer::DEFAULT_MAX_REDUNDANT_IPS = 4;
TimeVal SubnetInferrer::DEFAULT_PROBE_TIMEOUT_PERIOD(2, 0);
unsigned int SubnetInferrer::CONJECTURED_GLOBAL_INTERNET_DIAMETER = 64;
unsigned char SubnetInferrer::MIN_CORE_IP_SUBNET_PREFIX = 20;
unsigned char SubnetInferrer::MIN_CORE_IP_ALIAS_PREFIX = 28;
unsigned char SubnetInferrer::DEFAULT_SUBNET_INFERENCE_MIDDLE_TTL = 12;
bool SubnetInferrer::DEFAULT_STRICT_POSITIONING = false;
unsigned char SubnetInferrer::DEFAULT_MIDDLE_TTL = 12;

SubnetInferrer::SubnetInferrer(TreeNETEnvironment *e, 
                               unsigned short li, 
                               unsigned short ui, 
                               unsigned short ls, 
                               unsigned short us) throw (SocketException):
env(e)
{
    srand(time(0));
    
    temporaryProbingCost = 0;
    debug = env->debugMode();
    verbose = debug || env->getDisplayMode() == TreeNETEnvironment::DISPLAY_MODE_VERBOSE;
    
    unsigned short protocol = env->getProbingProtocol();
    
    if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_UDP)
    {
        int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
        if(env->usingFixedFlowID())
            roundRobinSocketCount = 1;
        
        prober = new DirectUDPWrappedICMPProber(env->getAttentionMessage(), 
                                                roundRobinSocketCount, 
                                                env->getTimeoutPeriod(), 
                                                env->getProbeRegulatingPeriod(), 
                                                li, 
                                                ui, 
                                                ls, 
                                                us, 
                                                env->debugMode());
    }
    else if(protocol == TreeNETEnvironment::PROBING_PROTOCOL_TCP)
    {
        int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
        if(env->usingFixedFlowID())
            roundRobinSocketCount = 1;
        
        prober = new DirectTCPWrappedICMPProber(env->getAttentionMessage(), 
                                                roundRobinSocketCount, 
                                                env->getTimeoutPeriod(), 
                                                env->getProbeRegulatingPeriod(), 
                                                li, 
                                                ui, 
                                                ls, 
                                                us, 
                                                env->debugMode());
    }
    else
    {
        prober = new DirectICMPProber(env->getAttentionMessage(), 
                                      env->getTimeoutPeriod(), 
                                      env->getProbeRegulatingPeriod(), 
                                      li, 
                                      ui, 
                                      ls, 
                                      us, 
                                      env->debugMode());
    }
    
    if(debug)
    {
        logStream << prober->getAndClearLog();
    }
}

SubnetInferrer::~SubnetInferrer()
{
    delete prober;
}

SubnetSite *SubnetInferrer::inferRemoteSubnet(const InetAddress &destinationAddress, 
                                              bool strictSubnetPositioning, 
                                              unsigned char middleTTL, 
                                              bool useLowerBorderAsWell) throw(NoTTLEstimationException, 
                                                                               UnresponsiveIPException, 
                                                                               UndesignatedPivotInterface, 
                                                                               ShortTTLException)
{
    SubnetSite *subnetSite = NULL;
    if(destinationAddress.isUnset())
    {
        throw InetAddressException("Unrecognizable target IP Address");
    }

    if(verbose)
    {
        logStream << "\n";
        logStream << "[Exploring the subnet accomodating " << destinationAddress << "]" << endl;
    }

    ProbeRecord *sitePrevPrevRecord = NULL;
    ProbeRecord *sitePrevRecord = NULL;
    ProbeRecord *siteRecord = NULL;
    try
    {
        bool recordPopulation = populateRecords(destinationAddress, 
                                                &siteRecord, 
                                                &sitePrevRecord, 
                                                &sitePrevPrevRecord, 
                                                strictSubnetPositioning, 
                                                middleTTL, 
                                                useLowerBorderAsWell);
        
        /*
         * TreeNET v2.1 (01/2016) and onwards: avoids inferring if IP is in a /20 range from 
         * "IPBlocksToAvoid" set found in the TreeNETEnvironment class. However, the TTL must be 
         * the same as the Pivot TTL for this range to exclude the destination IP for sure, hence 
         * the need to go through populateRecords(). To signal this to the calling environment, 
         * this method returns the subnet from IPBlocksToAvoid which contains the destination IP. 
         * There is no ambiguity since it has a refinement status other than NOT_PREPARED_YET.
         */
        
        if(siteRecord != NULL)
        {
            SubnetSiteSet *toAvoid = env->getIPBlocksToAvoid();
            SubnetSite *ss = toAvoid->getSubnetContainingWithTTL(destinationAddress, 
                                                                 siteRecord->getReqTTL());
            if(ss != NULL)
            {
                delete siteRecord;
                if(sitePrevRecord != NULL)
                    delete sitePrevRecord;
                if(sitePrevPrevRecord != NULL)
                    delete sitePrevPrevRecord;
                cache.clear();
                
                return ss;
            }
        }
        
        int subnetPositioningProbingCost = temporaryProbingCost;
        temporaryProbingCost = 0;
        if(recordPopulation)
        {
            subnetSite = exploreSite_B(destinationAddress, siteRecord, sitePrevRecord, sitePrevPrevRecord, useLowerBorderAsWell);
            subnetSite->spCost = subnetPositioningProbingCost;
        }
    }
    catch (NoTTLEstimationException &e)
    {
        delete siteRecord;
        delete sitePrevRecord;
        delete sitePrevPrevRecord;
        temporaryProbingCost = 0;
        cache.clear();
        throw e;
    }
    catch (UnresponsiveIPException &e)
    {
        delete siteRecord;
        delete sitePrevRecord;
        delete sitePrevPrevRecord;
        temporaryProbingCost = 0;
        cache.clear();
        throw e;
    }
    catch (UndesignatedPivotInterface &e) 
    {
        delete siteRecord;
        delete sitePrevRecord;
        delete sitePrevPrevRecord;
        temporaryProbingCost = 0;
        cache.clear();
        throw e;
    }
    catch (ShortTTLException &e) 
    {
        delete siteRecord;
        delete sitePrevRecord;
        delete sitePrevPrevRecord;
        temporaryProbingCost = 0;
        cache.clear();
        throw e;
    }

    subnetSite->siCost = this->temporaryProbingCost;
    
    delete siteRecord;
    delete sitePrevRecord;
    delete sitePrevPrevRecord;
    temporaryProbingCost = 0;
    cache.clear();

    return subnetSite;
}

bool SubnetInferrer::populateRecords(const InetAddress &dst,
                                     ProbeRecord **siteRecord,
                                     ProbeRecord **sitePrevRecord,
                                     ProbeRecord **sitePrevPrevRecord,
                                     bool strictSubnetPositioning,
                                     unsigned char middleTTL,
                                     bool useLowerBorderAsWell) throw (NoTTLEstimationException, 
                                                                       UnresponsiveIPException, 
                                                                       UndesignatedPivotInterface, 
                                                                       ShortTTLException)
{
    bool success = false;
    (*siteRecord) = NULL;
    (*sitePrevRecord) = NULL;
    (*sitePrevPrevRecord) = NULL;
    ProbeRecord *firstICMPEchoReply = NULL;
    ProbeRecord *rec = NULL;
    unsigned char TTL = middleTTL;
    unsigned short MAX_CONSECUTIVE_ANONYMOUS_COUNT = DEFAULT_MAX_CONSECUTIVE_ANONYMOUS;
    unsigned short MAX_REDUNDANT_IPS = DEFAULT_MAX_REDUNDANT_IPS;
    unsigned short consecutiveAnonymousCount = 0;
    unsigned short redundancies = 0;
    std::set<InetAddress> appearingIPset;
    unsigned char dstTTL = 0;
    
    /**
     * (for dstTTL) Introduced long after coding this function to fill destinationIPdistance 
     * field of UndesignatedPivotInterface. It does not have any other effect.
     */

    if(verbose)
    {
        logStream << "\n";
        logStream << "[Looking for a pivot IP, starting from target IP]" << endl;
        logStream << "Forward probing to evaluate distance from target IP in TTL..." << endl;
    }
    
    /**
     * Sends probes to dst with an increasing TTL (by default, TTL starts at 1), with the hope
     * of discovering the hop distance between the vantage point and dst. This step stops when
     * -we crossed too many consecutive anonymous destinations (i.e. their ICMP reply did not 
     *  contain source); this amount is given by a constant
     * -we obtained too many times identical responsive IPs along the path (responsive IPs are 
     *  stored in appearingIPset), hinting there is probably a routing cycle, in which case we 
     *  will avoid this IP (TreeNET v3.0)
     * -TTL is too high (limit is defined by a constant)
     * -(responding device is not anonymous) the IP of the responding device is already in a set 
     *  (refreshed as long as we do not cross anonymous devices; it is reset when crossing an 
     *  anonymous device): we are in a loop
     * -(responding device is not anonymous) we received an ICMP reply which is neither an echo,
     *  neither a TTL exceeded
     * -(responding device is not anonymous) we received an ICMP echo reply
     */
     
    bool stop = false;
    do
    {
        rec = probe(dst, TTL);

        if(rec->isAnonymousRecord())
            consecutiveAnonymousCount++;
        else
        {
            if(appearingIPset.find(rec->getRplyAddress()) == appearingIPset.end())
                appearingIPset.insert(rec->getRplyAddress());
            else
            {
                redundancies++;
                if(redundancies >= MAX_REDUNDANT_IPS)
                {
                    firstICMPEchoReply = NULL;
                    delete rec;
                    rec = NULL;
                    break;
                }
            }
            consecutiveAnonymousCount = 0;
            if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                firstICMPEchoReply = rec;
                rec = NULL;
                break;
            }
            else if(rec->getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                stop = true;
        }
        delete rec;
        rec = NULL;
        TTL++;
    }
    while(!stop && consecutiveAnonymousCount < MAX_CONSECUTIVE_ANONYMOUS_COUNT && firstICMPEchoReply == NULL 
          && TTL <= (unsigned char) CONJECTURED_GLOBAL_INTERNET_DIAMETER);

    /**
     * Next part is backward probing. This steps aims at correcting the TTL value if the TTL
     * computed at previous step is equal to the middleTTL (minimum TTL; default is 1). Indeed,
     * it is possible that the ICMP reply came from a device reachable with a TTL smaller than
     * middleTTL.
     */

    if(firstICMPEchoReply != NULL && (unsigned short) TTL > 1 && TTL == middleTTL)
    {
        if(verbose)
        {
            if(debug)
                logStream << endl;
            logStream << "Backward probing, as computed TTL is equal to the minimum hop count..." << endl;
        }
        
        TTL--;
        while(TTL > 1)
        {
            rec = probe(dst, TTL);
            if(!rec->isAnonymousRecord())
            {
                if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                {
                    delete firstICMPEchoReply;
                    firstICMPEchoReply = rec;
                    rec = NULL;
                }
                else
                {
                    delete rec;
                    rec = NULL;
                    break;
                }
            }
            else
            {
                delete rec;
                rec = NULL;
                break;
            }
            TTL--;
        }
    }
    
    if(verbose)
    {
        if(debug)
            logStream << endl;
        
        if(consecutiveAnonymousCount >= MAX_CONSECUTIVE_ANONYMOUS_COUNT)
        {
            logStream << "Too many consecutive anonymous replies (" << MAX_CONSECUTIVE_ANONYMOUS_COUNT;
            logStream << ") during TTL estimation." << endl;
        }
        else if(redundancies >= MAX_REDUNDANT_IPS)
        {
            logStream << "Too many redundant responsive IPs (" << MAX_REDUNDANT_IPS;
            logStream << "), hinting routing cycle. This target IP will be ignored." << endl;
        }
        else if(TTL > (unsigned char) CONJECTURED_GLOBAL_INTERNET_DIAMETER)
        {
            logStream << "Estimated TTL is too large (more than " << CONJECTURED_GLOBAL_INTERNET_DIAMETER << ")." << endl;
        }
        else
        {
            logStream << "Distance between this VP and target in TTL: " << (unsigned short) TTL << endl;
        }
    }
    
    // Registers the TTL for this IP
    IPTableEntry *IPEntry = env->getIPTable()->lookUp(dst);
    if(IPEntry != NULL)
    {
        IPEntry->setTTL(TTL);
    }
    
    // Stops probing if the program failed in obtaining an echo reply.
    if(firstICMPEchoReply == NULL)
    {
        if(rec != NULL)
            delete rec;
        std::string msg = "";
        if(consecutiveAnonymousCount >= MAX_CONSECUTIVE_ANONYMOUS_COUNT)
        {
            msg = "Could not estimate the distance in TTL to reach ";
            msg += *(dst.getHumanReadableRepresentation()) + ": too many consecutive anonymous replies.";
            throw NoTTLEstimationException(dst, msg);
        }
        else if(redundancies >= MAX_REDUNDANT_IPS)
        {
            msg = "Could not estimate the distance in TTL to reach ";
            msg += *(dst.getHumanReadableRepresentation()) + ": too many redundant responsive IPs.";
            throw NoTTLEstimationException(dst, msg);
        }
        else if(TTL > (unsigned char) CONJECTURED_GLOBAL_INTERNET_DIAMETER)
        {
            msg = "Could not estimate the distance in TTL to reach ";
            msg += *(dst.getHumanReadableRepresentation()) + ": estimated TTL is too large.";
            throw NoTTLEstimationException(dst, msg);
        }
        else
        {
            msg = "Target IP address " + *(dst.getHumanReadableRepresentation());
            msg += " is not responsive.";
            throw UnresponsiveIPException(dst, msg);
        }
    }
    
    /**
     * SUBNET POSITIONING (original comment from ExploreNET v2.1 sources)
     *
     * Dest<-------...--------oR3o<--------oR2o<--------oR1o<-----------...----------Vantage
     *                         o
     *                         |
     *                oR4o-----| SubnetB
     *                         |
     *                         o
     *                        oR5o
     *
     * Assume our siteRecord, sitePrevRecord and sitePrevPrevRecord are R3, R2, and R1 
     * respectively. If R3 returns its response via R3.south and the only path which goes to 
     * subnetB is through R3 then we must set the ingress of the site to R3.south and egress 
     * (pivot) of the site to one of the responsive /31 or /30 mates of the R3.south. As a result 
     * the site record TTLs are shifted (increased) by 1 compared to the site record TTLs.
     *
     * In order to tell whether we are in a such scenario we probe with 
     * firstICMPEchoReply->getReqTTL() /31 or /30 (when /31 is anonymous) mate of siteRecord. 
     * If we get a TIME_EXCEEDED, we are in the scenario.
     */
     
    dstTTL = firstICMPEchoReply->getReqTTL();
    if(verbose)
    {
        logStream << "Probing 31-mate/30-mate of target IP to check if it is a valid pivot IP ";
        logStream << "or a contra-pivot IP..." << endl;
    }
    InetAddress mate31 = firstICMPEchoReply->getDstAddress().get31Mate(); // rplyAddress might be different from probed address (dstAddress)
    rec = probe(mate31, firstICMPEchoReply->getReqTTL()); // Checks mate-31
    
    // Gets mate-30 if mate-31 is anonymous, unknown or unreachable
    if(rec->isAnonymousRecord() ||
       (!rec->isAnonymousRecord() && rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
       (rec->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNREACHABLE || rec->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNKNOWN)))
    {
        delete rec;
        rec = NULL;
        InetAddress mate30;
        try
        {
            mate30 = firstICMPEchoReply->getDstAddress().get30Mate();
        }
        catch(InetAddressException &e)
        {
            mate30 = mate31.get30Mate();
        }
        rec = probe(mate30, firstICMPEchoReply->getReqTTL());
    }

    /**
     * Record for mate-31 or mate-30 is not anonymous, but TTL exceeded: we need to 
     * re-position the subnet. We need new records and more exploration. We limit ourselves 
     * to /29 subnet exploration (/30 if we use strict subnet positioning).
     */
    
    if(!rec->isAnonymousRecord() && rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
    {
        if(verbose)
        {
            if(debug)
                logStream << endl;
            logStream << "No reply from 31-mate/30-mate of the target IP with its TTL." << endl;
            logStream << "Re-positioning is required. Trying to discover a new pivot IP..." << endl;
        }
        
        unsigned char maxSearchPrefix = 30;
        if(!strictSubnetPositioning)
            maxSearchPrefix = 29;
    
        unsigned char currentSearchPrefix = 31;
        InetAddress firstIP, lastIP, currentIP;
        bool stopSearch = false;
        delete rec;
        rec = NULL;

        appearingIPset.clear();
        appearingIPset.insert(firstICMPEchoReply->getDstAddress());

        while(!stopSearch && (*siteRecord) == 0 && currentSearchPrefix >= maxSearchPrefix)
        {
            // Temporar subnet with prefix /currentSearchPrefix
            NetworkAddress tmp(firstICMPEchoReply->getDstAddress(), currentSearchPrefix);
            if(currentSearchPrefix == 31)
            {
                firstIP = tmp.getLowerBorderAddress();
                lastIP = tmp.getUpperBorderAddress();
            }
            else
            {
                firstIP = tmp.getLowerBorderAddress();
                if(!useLowerBorderAsWell)
                    firstIP++;
                lastIP = tmp.getUpperBorderAddress() - 1;
            }
            
            /*
             * N.B.: theoretically, at this point, there is no risk of encompassing LAN.
             */
            
            if(verbose)
            {
                logStream << "The following address block will be investigated: " << tmp << "." << endl;
            }
            
            IPLookUpTable *table = env->getIPTable();
            for(currentIP = firstIP; currentIP <= lastIP; currentIP++)
            {
                // TreeNET v2.0 and onwards (Oct 8, 2015): skips IP which are known to be unresponsive.
                if(table->lookUp(currentIP) == NULL)
                {
                    if(verbose)
                    {
                        logStream << currentIP << " skipped because unresponsive at pre-scanning." << endl;
                    }
                    continue;
                }
                
                // If currentIP is not in the set, we probe it (TTL+1) and check results.
                if(appearingIPset.find(currentIP) == appearingIPset.end())
                {
                    if(verbose)
                    {
                        logStream << "Checking " << currentIP << "..." << endl;
                    }
                
                    appearingIPset.insert(currentIP);
                    rec = probe(currentIP, firstICMPEchoReply->getReqTTL() + 1);
                    
                    if(!rec->isAnonymousRecord())
                    {
                        // currentIP with TTL+1 is not anonymous and sent an echo reply
                        if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                        {
                            // currentIP is mate-31 with first echo replying node: it is the new pivot
                            if(currentIP == firstICMPEchoReply->getDstAddress().get31Mate())
                            {
                                (*siteRecord) = rec;
                                (*siteRecord)->setRplyAddress(currentIP); // In case currentIP differs from (*siteRecord)->getDstAddress()
                                rec = NULL;
                                
                                if(verbose)
                                {
                                    if(debug)
                                        logStream << endl;
                                    logStream << currentIP << " replied to probe with TTL+1 and is mate-31; it is the new pivot IP." << endl;
                                }
                                
                                break;
                            }
                            // Additionnal probes
                            else
                            {
                                ProbeRecord *tmpRec = NULL;
                                tmpRec = probe(currentIP, firstICMPEchoReply->getReqTTL());
                                
                                // currentIP is at same distance; it is not on the subnet: we stop
                                if(!tmpRec->isAnonymousRecord() && tmpRec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                                {
                                    delete rec;
                                    rec = NULL;
                                    delete tmpRec;
                                    tmpRec = NULL;
                                    stopSearch = true;
                                    
                                    if(verbose)
                                    {
                                        if(debug)
                                            logStream << endl;
                                        logStream << currentIP << " is at the same TTL as initial target. Re-positioning stops now." << endl;
                                    }
                                    
                                    break;
                                }
                                delete tmpRec;
                                tmpRec = NULL;
                                tmpRec = probe(currentIP.get31Mate(), firstICMPEchoReply->getReqTTL() + 1);
                                
                                // 31-mate of currentIP is out of range; it is not on the subnet
                                if(!tmpRec->isAnonymousRecord() && tmpRec->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                                {
                                    delete rec;
                                    rec = NULL;
                                    delete tmpRec; 
                                    tmpRec = NULL;
                                    stopSearch = true;
                                    
                                    if(verbose)
                                    {
                                        if(debug)
                                            logStream << endl;
                                        logStream << "Mate-31 of " << currentIP << " is out or range. Re-positionng stops now." << endl;
                                    }
                                    
                                    break;
                                }
                                delete tmpRec;
                                tmpRec = NULL;
                                
                                if(verbose)
                                {
                                    if(debug)
                                        logStream << endl;
                                    logStream << currentIP << " replied to probe with TTL+1 and is on the subnet;";
                                    logStream << " it is the new pivot IP." << endl;
                                }
                                
                                // currentIP is a valid pivot
                                (*siteRecord) = rec;
                                (*siteRecord)->setRplyAddress(currentIP); // In case currentIP differs from (*siteRecord)->getDstAddress()
                                rec = NULL;
                                break;
                            }
                        }
                        // Stops if we exceeded TTL (actually TTL+1)
                        else if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                        {
                            delete rec;
                            rec = NULL;
                            stopSearch = true;
                            
                            if(verbose)
                            {
                                if(debug)
                                    logStream << endl;
                                logStream << currentIP << " is out of range (ICMP type time exceeded). ";
                                logStream << "Re-positioning stops now." << endl;
                            }
                            
                            break;
                        }
                    }
                    
                    // Since currentIP is an anonymous node, deletes rec
                    delete rec;
                    rec = NULL;
                }
                else if(verbose)
                {
                    logStream << currentIP << " ignored because already checked before." << endl;
                }
            }
            currentSearchPrefix--;
        }
    }
    // We do not need to re-position the site
    else
    {
        if(verbose)
        {
            logStream << "No re-positioning is needed." << endl;
        }
        (*siteRecord) = firstICMPEchoReply;
        (*siteRecord)->setRplyAddress(firstICMPEchoReply->getDstAddress()); // If rplyAddress differs from dstAddress
        
        firstICMPEchoReply = NULL;
    }
    delete firstICMPEchoReply; 
    firstICMPEchoReply = NULL;
    
    if(rec != NULL)
    {
        delete rec;
        rec = NULL;
    }
    
    // Valid pivot
    if((*siteRecord) != NULL && !(*siteRecord)->isAnonymousRecord())
    {
        unsigned char siteRecordTTL = (*siteRecord)->getReqTTL();
        
        // Pivot is too close
        if(siteRecordTTL < 2)
        {
            delete (*siteRecord);
            (*siteRecord) = NULL;
            
            std::string msg = "Pivot IP address \"" + *(dst.getHumanReadableRepresentation());
            msg += "\" should at least be located at 2 hops distance away from the vantage point";
            throw ShortTTLException(dst, msg);
        }
        // Success! Probes sitePrevRecord and sitePrevPrevRecord with TTL-1 and TTL-2
        else
        {
            success = true;
            if(verbose)
            {
                logStream << "Probing pivot IP (distance: TTL) with TTL-1 and TTL-2..." << endl;
            }
            (*sitePrevRecord) = probe((*siteRecord)->getDstAddress(), siteRecordTTL - 1);
            if(siteRecordTTL > 2)
            {
                (*sitePrevPrevRecord) = probe((*siteRecord)->getDstAddress(), siteRecordTTL - 2);
            }
            // Additionnal line break before the populated records
            if(debug)
                logStream << endl;
        }
    }
    // Anonymous pivot or no pivot at all: throws an exception
    else
    {
        if((*siteRecord) != NULL)
        {
            delete (*siteRecord); 
            (*siteRecord) = NULL;
        }
        
        std::string msg = "Undesignated pivot interface for the target site hosting IP address \"";
        msg += *(dst.getHumanReadableRepresentation())+"\"";
        throw UndesignatedPivotInterface(dst, dstTTL, msg);
    }

    if(verbose)
    {
        logStream << "Populated records (target is pivot IP, TTL is #hops to reach it):\n";
        logStream << "Replying IP at TTL:   ";
        logStream << ((*siteRecord) == 0 ? "NULL" : *((*siteRecord)->getDstAddress().getHumanReadableRepresentation()));
        logStream << "\nReplying IP at TTL-1: ";
        logStream << ((*sitePrevRecord) == 0 ? "NULL" : *((*sitePrevRecord)->getRplyAddress().getHumanReadableRepresentation()));
        logStream << "\nReplying IP at TTL-2: ";
        logStream << ((*sitePrevPrevRecord) == 0 ? "NULL": *((*sitePrevPrevRecord)->getRplyAddress().getHumanReadableRepresentation()));
        logStream << endl << endl;
    }

    return success;
}

SubnetSite *SubnetInferrer::exploreSite_B(const InetAddress &dst,
                                          ProbeRecord *siteRecord,
                                          ProbeRecord *sitePrevRecord,
                                          ProbeRecord *sitePrevPrevRecord,
                                          bool useLowerBorderAsWell)
{
    if(verbose)
    {
        logStream << "[Subnet exploration]" << endl;
    }

    /**
     * If the record is ANONYMOUS then we cannot evaluate /x of the IP belongs to record.
     * Hence, we can neither discover alias nor explore subnet.
     */
    
    if(siteRecord == 0 || siteRecord->isAnonymousRecord())
    {
        return 0;
    }

    /**
     * Important note: aliasRecord and alias objects refer to the contra-pivot interface in the 
     * paper describing ExploreNET. They are called alias here because contra-pivot interface 
     * and the sitePrevRecord IP addresses should be hosted by the same router.
     */
    
    ProbeRecord *aliasRecord = NULL;
    InetAddress alias; // By default it is undefined i.e. 0

    enum SubnetSiteNode::AliasDiscoveryMethod aliasDiscoveryMethod = SubnetSiteNode::UNKNOWN_ALIAS_SX;
    // Unsigned char aliasPrefix = 0;

    unsigned char subnetPrefix = 31;
    // Contra-pivot distance from vantage point (we use this instead of getting directly from sitePrevRecord because it might be NULL)
    unsigned char subnetdTTL = siteRecord->getReqTTL() - 1;
    bool subnetMate3130QuotaFilled = false;
    bool aliasMate3130QuotaFilled = false;
    bool rule3, rule4, rule5;
    InetAddressSet probedIPset;
    ProbeRecord *newRecorddTTLPlus1 = NULL;
    ProbeRecord *newRecorddTTL = NULL;
    SubnetSite *subnetSite = new SubnetSite();
    subnetSite->targetIPaddress = dst;
    subnetSite->pivotIPaddress = siteRecord->getDstAddress();
    subnetSite->prevSiteIPaddress = sitePrevRecord->getRplyAddress();
    subnetSite->prevSiteIPaddressDistance = sitePrevRecord->getReqTTL();

    try
    {
        /**
         * The IP of the record is always the first member of the subnet so let us immediately 
         * store it into the subnet with prefix 32.
         */
        
        subnetSite->insert(new SubnetSiteNode(siteRecord->getDstAddress(), 32, subnetdTTL + 1, SubnetSiteNode::UNKNOWN_ALIAS_SX));
        probedIPset.insert(new InetAddress(siteRecord->getDstAddress()));
        InetAddress firstAddress; // For lower border adddress of the subnet
        InetAddress lastAddress; // For upper border address of the subnet

        /**
         * This loop decreases subnetPrefix at each pass, until a minimum is reached (which is
         * determined by a constant). The body of such loop aims at verifying if we can grow
         * the subnet or if we should stop.
         */
        
        bool skipped = false; // To adjust line breaks in verbose mode
        while(subnetPrefix >= MIN_CORE_IP_SUBNET_PREFIX)
        {
            // Computes the size of the current subnet and checks it
            if(subnetPrefix < 29)
            {
                int thresholdSubnetSize;
                if(!useLowerBorderAsWell)
                    thresholdSubnetSize = (int) ((pow(2, 32 - (subnetPrefix + 1)) - 2) / 2);
                else
                    thresholdSubnetSize = (int) (((pow(2, 32 - (subnetPrefix + 1)) - 2) / 2) + 1);

                if(subnetSite->getTotalSize() < thresholdSubnetSize)
                {
                    if(verbose)
                    {
                        if(skipped)
                        {
                            skipped = true;
                            logStream << endl;
                        }
                        logStream << "Stopping growing the subnet because there ";
                        if(subnetSite->getTotalSize() > 1)
                            logStream << "are " << subnetSite->getTotalSize() << " IPs ";
                        else
                            logStream << "is a single IP ";
                        logStream << "there should have been ";
                        logStream << thresholdSubnetSize + 1 << " to move from /" << (int) subnetPrefix + 1 << " to ";
                        logStream << (int) subnetPrefix << "." << endl;
                    }
                    break; // STOP growing subnet further
                }
            }

            // Current subnet
            NetworkAddress subnet(siteRecord->getDstAddress(), subnetPrefix);

            if(subnetPrefix == 31)
            {
                firstAddress = subnet.getLowerBorderAddress();
                lastAddress = subnet.getUpperBorderAddress();
            }
            else
            {
                firstAddress = subnet.getLowerBorderAddress();
                if(!useLowerBorderAsWell)
                    firstAddress++;
                
                lastAddress = subnet.getUpperBorderAddress() - 1;
                
                // Checks that new subnet does not collide with LAN (Oct 15, 2015)
                NetworkAddress LAN = env->getLAN();
                InetAddress LANLowerBound = LAN.getLowerBorderAddress();
                InetAddress LANUpperBound = LAN.getUpperBorderAddress();
                if(firstAddress <= LANLowerBound && lastAddress >= LANUpperBound)
                {
                    if(verbose)
                    {
                        if(skipped)
                        {
                            skipped = true;
                            logStream << endl;
                        }
                        logStream << "Stopping growing the subnet because prefix /";
                        logStream << (unsigned short) subnetPrefix + 1 << " encompasses LAN. /";
                        logStream << (unsigned short) subnetPrefix << " will be kept." << endl;
                    }
                    break; // STOP growing subnet further
                }
            }
            
            if(verbose)
            {
                if(skipped)
                {
                    skipped = false;
                    logStream << endl;
                }
                logStream << "Evaluating the consistency of /" << (unsigned short) subnetPrefix << "...\n" << endl;
            }

            // Second loop to check all addresses within the subnet
            IPLookUpTable *table = env->getIPTable();
            for(InetAddress currentAddress = firstAddress; currentAddress <= lastAddress; currentAddress++)
            {
                // TreeNET v2.0 and onwards (exact date: Oct 8, 2015): skips IPs known to be unresponsive
                if(table->lookUp(currentAddress) == NULL)
                {
                    if(verbose)
                    {
                        logStream << "Skipped candidate IP " << currentAddress << " because known ";
                        logStream << "to be unresponsive through pre-scanning phase." << endl;
                        
                        skipped = true;
                    }
                    continue;
                }
            
                if(!probedIPset.contains(currentAddress))
                {
                    probedIPset.insert(new InetAddress(currentAddress));
                    if(verbose)
                    {
                        if(skipped)
                        {
                            skipped = false;
                            logStream << endl;
                        }
                        logStream << "Analyzing candidate IP " << currentAddress << "..." << endl;
                    }
                    newRecorddTTLPlus1 = probe(currentAddress, subnetdTTL + 1); // NEW PROBE
                    if(!newRecorddTTLPlus1->isAnonymousRecord())
                    {
                        // Additionnal line break for harmonious display
                        if(debug)
                            logStream << endl;
                    
                        // Apply grow subnet rule 1
                        if(growSubnet_Rule_1_B(newRecorddTTLPlus1, subnetPrefix))
                        {
                            /**
                             * (following comment originates from ExploreNET v2.1 sources)
                             *
                             * An important element which needs care is newRecorddTTL which is the 
                             * result of probing current address with subnetdTTL. It is used in 
                             * i) alias discovery ii) growSubnet RULE-3. Until an alias is found 
                             * we set this variable within alias discovery scope. After it has 
                             * been found we set it just before calling RULE-3 method.
                             *
                             * Moreover we must take special care about the /3130 quota of growing 
                             * subnet and alias discovery. That is /31 or a /30 (if /31 is NOT 
                             * responsive) mate is added to the subnet list and accepted as alias 
                             * without further examination in order to reduce probing overhead.
                             * We must keep two variables one for subnet and one for alias. 
                             * Filling aliasMate3130Quota implies subnetMate3130Quota has been 
                             * filled. But the reverse is not true. There might be cases a mate-31 
                             * did returned TIME_EXCEEDED for probe with dTTL (not alias), yet it 
                             * is definetly a member of the subnet and maybe /30 will be the alias.
                             */

                            /*
                             * START ALIAS DISCOVERY
                             *
                             * Reminder by J.-F. Grailet: this "alias discovery" step is actually 
                             * more like a step to test if the current IP is a valid contra-pivot 
                             * interface for the subnet. In a sense, it is an alias, because 
                             * theoretically, a contra-pivot interface of a subnet should be 
                             * located on the same device as the IP found in "sitePrevRecord". 
                             * Therefore, this IP and the contra-pivot are alias of each other.
                             *
                             * It is important to note that this does not always hold true, 
                             * because constructions such as meshes mixing up L3/L2 devices will 
                             * end up in having the IP from "sitePrevRecord" on a completely 
                             * different device. A subsequent step in TreeNET aim at providing a 
                             * much more elaborated, complete alias resolution method. It also 
                             * provides a subnet refinement phase to ensure the presence of one 
                             * or several contra-pivot candidates in each subnet and evaluates 
                             * the soundness of an inferred subnet on that basis.
                             *
                             * It is indeed possible, in some configurations, to have more than 
                             * one contra-pivot due to the usage of back-up interfaces to 
                             * compensate potential failures of another interface. A subnet 
                             * featuring several contra-pivot candidates is considered as "odd", 
                             * while a subnet with a single candidate is considered as "accurate". 
                             * Subnets for which a contra-pivot could not be found at all are 
                             * labelled as "shadow" subnets (we know a subnet exist, but we cannot 
                             * be certain on its size and connection with the rest of the network).
                             */
                            
                            if(sitePrevRecord != NULL && !sitePrevRecord->isAnonymousRecord() && aliasRecord == 0)
                            {
                                bool foundSomething = false;
                            
                                if(verbose)
                                {
                                    logStream << "Start of alias discovery, i.e., checking if ";
                                    logStream << "current IP is a (sound) contra-pivot ";
                                    logStream << "interface..." << endl;
                                }
                                
                                newRecorddTTL = probe(currentAddress, subnetdTTL); // NEW PROBE
                                // To add a space after the previous probe record
                                if(debug)
                                    logStream << endl;
                                
                                if(!newRecorddTTL->isAnonymousRecord() && newRecorddTTL->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                                {
                                    if(discoverAlias_Rule_1_B(sitePrevRecord, newRecorddTTL))
                                    {
                                        aliasDiscoveryMethod = SubnetSiteNode::SOURCE_BASED_ALIAS_SX;
                                    }
                                    else if(subnetPrefix >= 30 && aliasMate3130QuotaFilled == false && 
                                            discoverAlias_Rule_2_B(siteRecord, currentAddress, subnetPrefix))
                                    {
                                        aliasDiscoveryMethod = SubnetSiteNode::MATE3031_BASED_ALIAS_SX;
                                        aliasMate3130QuotaFilled = true;
                                        subnetMate3130QuotaFilled = true;
                                        // subnetMate3130QuotaFilled might already be true! In that case, this does not make a difference
                                    }
                                    else if(discoverAlias_Rule_3_B(sitePrevRecord, currentAddress, subnetPrefix, subnetdTTL))
                                        aliasDiscoveryMethod = SubnetSiteNode::PALMTREE_BASED_ALIAS_SX;
                                    else if(discoverAlias_Rule_4_B(sitePrevRecord, sitePrevPrevRecord, currentAddress, subnetPrefix, subnetdTTL))
                                        aliasDiscoveryMethod = SubnetSiteNode::DISTANCE_BASED_ALIAS_SX;

                                    if(aliasDiscoveryMethod != SubnetSiteNode::UNKNOWN_ALIAS_SX)
                                    {
                                        if(verbose)
                                        {
                                            skipped = false;
                                            logStream << currentAddress << " is a sound contra-pivot interface.\n" << endl;
                                        }
                                        aliasRecord = new ProbeRecord(*newRecorddTTL);
                                        alias = currentAddress;
                                        // AliasPrefix = subnetPrefix;
                                        subnetSite->insert(new SubnetSiteNode(currentAddress, subnetPrefix, subnetdTTL, aliasDiscoveryMethod));
                                        foundSomething = true;
                                        
                                        delete newRecorddTTLPlus1;
                                        newRecorddTTLPlus1 = NULL;
                                        delete newRecorddTTL;
                                        newRecorddTTL = NULL;
                                        continue;
                                    }
                                }
                                
                                /**
                                 * End of if(!newRecorddTTL->isAnonymousRecord() && 
                                 * newRecorddTTL->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                                 */
                                
                                if(verbose)
                                {
                                    if(!foundSomething)
                                    {
                                        logStream << currentAddress << " is not a sound contra-pivot candidate." << endl;
                                    }
                                }
                            }
                            else
                            {
                                if(verbose)
                                {
                                    logStream << "Skipping alias discovery because ";
                                    if(aliasRecord != NULL)
                                        logStream << "a contra-pivot interface has already been found." << endl;
                                    else
                                        logStream << "because the IP at TTL - 1 is anonymous." << endl;
                                }
                            }

                            // END ALIAS DISCOVERY

                            if(subnetPrefix >= 30 && subnetMate3130QuotaFilled == false)
                            {
                                subnetMate3130QuotaFilled = growSubnet_Rule_2_B(siteRecord, currentAddress, subnetPrefix);
                                if(subnetMate3130QuotaFilled)
                                {
                                    if(verbose)
                                    {
                                        skipped = false;
                                        logStream << currentAddress << " is in the subnet.\n" << endl;
                                    }
                                    subnetSite->insert(new SubnetSiteNode(currentAddress, subnetPrefix, 
                                                                          subnetdTTL + 1, SubnetSiteNode::UNKNOWN_ALIAS_SX));
                                    
                                    delete newRecorddTTLPlus1;
                                    newRecorddTTLPlus1 = NULL;
                                    delete newRecorddTTL;
                                    newRecorddTTL = NULL;
                                    continue;
                                }
                            }

                            if(newRecorddTTL == 0)
                                newRecorddTTL = probe(currentAddress, subnetdTTL); // NEW PROBE
                            rule3 = growSubnet_Rule_3_B(sitePrevRecord, newRecorddTTL, subnetPrefix);
                            if(rule3 == true)
                            {
                                rule4 = growSubnet_Rule_4_B(currentAddress, subnetPrefix, subnetdTTL);
                                if(rule4 == true)
                                {
                                    rule5 = growSubnet_Rule_5_B(currentAddress, alias, subnetPrefix, subnetdTTL);
                                    if(rule5 == true) // Means rule5, rule4, and rule3 are true
                                    {
                                        if(verbose)
                                        {
                                            skipped = false;
                                            logStream << currentAddress << " is in the subnet.\n" << endl;
                                        }
                                        subnetSite->insert(new SubnetSiteNode(currentAddress, 
                                                                              subnetPrefix, 
                                                                              subnetdTTL+1, 
                                                                              SubnetSiteNode::UNKNOWN_ALIAS_SX));
                                        
                                        delete newRecorddTTLPlus1; 
                                        newRecorddTTLPlus1 = NULL;
                                        delete newRecorddTTL;
                                        newRecorddTTL = NULL;
                                        continue;
                                    }
                                }
                            }
                            delete newRecorddTTL;
                            newRecorddTTL = NULL;
                        }
                        // End of if(growSubnet_Rule_1_B(newRecord,subnetPrefix))
                    }
                    else if(verbose)
                    {
                        logStream << "Skipping this candidate: got an anonymous record after probing with pivot TTL." << endl;
                    }
                    delete newRecorddTTLPlus1; 
                    newRecorddTTLPlus1 = NULL;
                    
                    logStream << endl;
                }
                // End of if(!probedIPset.contains(currentAddress))
            }
            // End of for
            subnetPrefix--;
        }
        // End of while
    }
    catch(SubnetBarrierException &e)
    {
        delete newRecorddTTLPlus1;
        newRecorddTTLPlus1 = NULL;
        delete newRecorddTTL;
        newRecorddTTL = NULL;
        
        subnetSite->markSubnetOvergrowthElements(e.barrierPrefix, SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
        if(verbose)
        {
            logStream << "Subnet has been shrinked to /" << (unsigned short) e.barrierPrefix << "." << endl;
        }
    }

    probedIPset.hardReleaseMemory();

    subnetSite->adjustRemoteSubnet2(useLowerBorderAsWell);

    if(aliasRecord != NULL)
    {
        delete aliasRecord;
        aliasRecord = NULL;
    }
    return subnetSite;
}

bool SubnetInferrer::discoverAlias_Rule_1_B(const ProbeRecord *sitePrevRecord, const ProbeRecord *newRecorddTTL)
{
    if(verbose)
    {
        logStream << "[Alias discovery rule n°1] Comparing replies at TTL - 1 for ";
        logStream << newRecorddTTL->getDstAddress() << " and ";
        logStream << sitePrevRecord->getDstAddress() << "..." << endl;
    }

    bool success = false;
    if(newRecorddTTL->getRplyAddress() == sitePrevRecord->getRplyAddress())
    {
        if(verbose)
        {
            logStream << "Success: replying IP is the same in both records." << endl;
        }
        success = true;
    }
    else if(verbose)
    {
        logStream << "Failure: replying IP is distinct in both records." << endl;
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_2_B(const ProbeRecord *siteRecord,
                                         const InetAddress &currentAddress,
                                         unsigned char currentSubnetPrefix) throw (SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Subnet growth rule n°2] Is " << currentAddress << " mate-30/mate-31 of ";
        logStream << siteRecord->getDstAddress() << " ?" << endl;
    }
    
    bool success = false;
    if(currentSubnetPrefix == 31)
    {
        if(siteRecord->getDstAddress().is31Mate(currentAddress))
        {
            if(verbose)
            {
                logStream << "Success: " << currentAddress << " is mate-31 of ";
                logStream << siteRecord->getDstAddress() << "." << endl;
            }
            success = true;
        }
        else if(verbose)
        {
            logStream << "Failure: while in a /31 subnet, " << currentAddress << " is not ";
            logStream << "mate-31 of " << siteRecord->getDstAddress() << "." << endl;
        }
    }
    else if(currentSubnetPrefix == 30)
    {
        try
        {
            if(siteRecord->getDstAddress().is30Mate(currentAddress))
            {
                if(verbose)
                {
                    logStream << "Success: " << currentAddress << " is mate-30 of ";
                    logStream << siteRecord->getDstAddress() << "." << endl;
                }
                success = true;
            }
            else if(verbose)
            {
                logStream << "Failure: while in a /30 subnet, " << currentAddress << " is not ";
                logStream << "mate-30 of " << siteRecord->getDstAddress() << "." << endl;
            }
        }
        catch (InetAddressException &e)
        {
            // Does nothing, siteRecord->getDstAddress() can NOT have a /30mate because it is a broadcast/network address.
            if(verbose)
            {
                logStream << "Failure: " << siteRecord->getDstAddress() << " cannot have a ";
                logStream << "mate-30 because it is a broadcast/network address." << endl;
            }
        }
    }
    else if(verbose)
    {
        logStream << "Failure: this rule is only applicable for /31 and /30 subnets." << endl;
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_3_B(const ProbeRecord *sitePrevRecord,
                                         const ProbeRecord *newRecorddTTL,
                                         unsigned char currentSubnetPrefix) throw (SubnetBarrierException)
{
    if(verbose)
    {
        if(debug)
            logStream << endl;
        logStream << "[Subnet growth rule n°3] Comparing probe replies at subnet TTL - 1 ";
        logStream << "for " << sitePrevRecord->getDstAddress() << " and ";
        logStream << newRecorddTTL->getDstAddress() << "..." << endl;
    }

    bool success = false;
    if(newRecorddTTL->isAnonymousRecord())
    {
        if(sitePrevRecord->isAnonymousRecord())
        {
            if(verbose)
            {
                logStream << "Success: both replies are anonymous." << endl;
            }
            success = true;
        }
        else if(verbose)
        {
            logStream << "Failure: reply when targetting " << sitePrevRecord->getDstAddress() << " is anonymous." << endl;
        }
    }
    else if(newRecorddTTL->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
    {
        if(!sitePrevRecord->isAnonymousRecord() && sitePrevRecord->getRplyAddress() == newRecorddTTL->getRplyAddress())
        {
            if(verbose)
            {
                logStream << "Success: replying IP for both probes is the same." << endl;
            }
            success = true;
        }
        else
        {
            /**
             * Some other prevRecord has responded to the query. Hence either the router 
             * prevPrevRecord is applying load balancing or the currentAddress is on the same 
             * dTTL+1 orbit but far from our site.
             */
            
            if(verbose)
            {
                logStream << "Failure: replying IP for each probe is distinct, therefore IPs are most probably from a distinct site." << endl;
            }
            throw SubnetBarrierException(currentSubnetPrefix + 1);
        }
    }
    else
    {
        /**
         * The response is neither anonymous nor ICMP TIME EXCEEDED having an ICMP ECHO_REPLY 
         * at this point implies that we found the second contra-pivot interface because if were 
         * first contra-pivot then it would have been discovered in ALIAS-DISCOVERY portion of 
         * the code.
         */
        
        if(verbose)
        {
            logStream << "Failure: the reply of the probe to " << newRecorddTTL->getDstAddress();
            logStream << " is neither anonymous neither Time exceeded." << endl;
        }
        throw SubnetBarrierException(currentSubnetPrefix + 1);
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_4_B(const InetAddress &currentAddress, 
                                         unsigned char currentSubnetPrefix, 
                                         unsigned char subnetdTTL) throw (SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Subnet growth rule n°4] Checking that mate-31/mate-30 of ";
        logStream << currentAddress << " is within reach with subnet TTL ..." << endl;
    }

    bool success = false;
    InetAddress mate31 = currentAddress.get31Mate();
    ProbeRecord *newRecord = probe(mate31, subnetdTTL + 1);
    if(newRecord->isAnonymousRecord() || (!newRecord->isAnonymousRecord() && 
       newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
       (newRecord->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNREACHABLE || 
        newRecord->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNKNOWN)))
    {
        delete newRecord;
        InetAddress mate30;
        try
        {
            mate30 = currentAddress.get30Mate();
        }
        catch(InetAddressException &e)
        {
            mate30 = mate31.get30Mate();
        }
        newRecord = probe(mate30, subnetdTTL + 1);
    }

    if(newRecord->isAnonymousRecord() || newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
    {
        if(verbose)
        {
            if(debug)
                logStream << endl;
            logStream << "Success: mate-30/mate-31 of " << currentAddress << " replied with an ";
            logStream << "ICMP Echo reply." << endl;
        }
        success = true;
    }
    else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
    {
        delete newRecord;
        newRecord = NULL;
        if(verbose)
        {
            if(debug)
                logStream << endl;
            logStream << "Failure: mate-30/mate-31 of " << currentAddress << " replied with a ";
            logStream << "Time exceeded reply." << endl;
        }
        throw SubnetBarrierException(currentSubnetPrefix + 1);
    }
    else if(verbose)
    {
        if(debug)
            logStream << endl;
        logStream << "Failure: mate-30/mate-31 of " << currentAddress << " replied with neither ";
        logStream << "a Time exceeded reply, neither an ICMP Echo reply." << endl;
    }
    delete newRecord;
    newRecord = NULL;
    
    return success;
}

bool SubnetInferrer::growSubnet_Rule_5_B(const InetAddress &currentAddress, 
                                         const InetAddress &alias, 
                                         unsigned char currentSubnetPrefix, 
                                         unsigned char subnetdTTL) throw (SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Subnet growth rule n°5] Checking the result of probing mate-31/mate-30 of ";
        logStream << currentAddress << " with subnet TTL - 1." << endl;
    }

    InetAddress mate31 = currentAddress.get31Mate();
    bool success = false;
    if(!alias.isUnset()) // We have a valid alias
    {
        if(mate31 == alias)
        {
            if(verbose)
            {
                logStream << "Success: mate-31 of " << currentAddress << " is " << alias;
                logStream << ", which is an alias of the same IP." << endl;
            }
            success = true;
        }
        else
        {
            ProbeRecord *newRecord = probe(mate31, subnetdTTL);
            if(newRecord->isAnonymousRecord() ||
               (!newRecord->isAnonymousRecord() && newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
                (newRecord->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNREACHABLE || 
                 newRecord->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNKNOWN)))
            {
                delete newRecord;
                InetAddress mate30;
                try
                {
                    mate30 = currentAddress.get30Mate();
                }
                catch(InetAddressException &e)
                {
                    mate30 = mate31.get30Mate();
                }
                newRecord = probe(mate30, subnetdTTL);
            }

            if(newRecord->isAnonymousRecord() || newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Success: mate-31/mate-30 of " << currentAddress << " is not an ";
                    logStream << " alias of the same IP and did not reply with an ICMP Echo ";
                    logStream << "reply with subnet TTL - 1." << endl;
                }
                success = true;
            }
            else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newRecord;
                newRecord = NULL;
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Failure: mate-31/mate-30 of " << currentAddress << " is not an ";
                    logStream << " alias of the same IP and replied with ICMP Echo reply while ";
                    logStream << "probing with subnet TTL - 1." << endl;
                }
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
            else if(verbose)
            {
                if(debug)
                    logStream << endl;
                logStream << "Failure: probing mate-31/mate-30 of " << currentAddress << " with ";
                logStream << "subnet TTL - 1 did result neither in a Time exceeded, neither in ";
                logStream << "an ICMP Echo reply, neither in an anonymous reply." << endl;
            }
            delete newRecord;
            newRecord = NULL;
        }
    }
    // We do not have a valid alias (That is it is NOT found yet)
    else
    {
        /**
         * exploreSubnet() method always first attempt to figure out whether the currentAddress is 
         * the alias. Also, it investigates the IP addresses in ascending order (from lower bound 
         * to upper bound). In case the currentAddress<currentAddress.mate31; we have not reached 
         * mate31 to investigate whether or not it is an alias. If mate31 really is an alias then 
         * having an ECHO_REPLY from it is totally OK. Now we have two options;
         * 1 - currentAddress < mate31: we simply return true (get current address added to the 
         * subnet), because next time mate31 will be investigated by exploreSubnet()  and if it is 
         * not the alias, it will raise SubnetBarrierPrefixException and cause this currentaddress 
         * as well as itself to be removed from the SiteSubnet on the other hand if it is alias 
         * everything is all right.
         * 2 - currentAddress > mate31: then it means mate31 has been explored before and it is 
         * decided not to be an alias. However, it could be part of the subnet or not. So again if 
         * we probe the mate31 with subnetdTTL we expect not to get an ICMP echo reply if we get 
         * than we raise SubnetBarrierException.
         */
        
        if(currentAddress < mate31)
        {
            if(verbose)
            {
                logStream << "Success (temporar): no alias is currently known for ";
                logStream << currentAddress << ", which comes before its mate-31 in IPv4 ";
                logStream << "addressing." << endl;
            }
            success = true;
        }
        else
        {
            ProbeRecord *newRecord = probe(mate31, subnetdTTL);
            if(newRecord->isAnonymousRecord() || newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Success: mate-31/mate-30 of " << currentAddress << " (not an ";
                    logStream << "alias) did not reply with an ICMP Echo reply with subnet ";
                    logStream << "TTL - 1." << endl;
                }
                success = true;
            }
            else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newRecord;
                newRecord = NULL;
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Failure: mate-31/mate-30 of " << currentAddress << " (not an ";
                    logStream << "alias) replied with an ICMP Echo reply with subnet TTL - 1.";
                }
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
            else if(verbose)
            {
                if(debug)
                    logStream << endl;
                logStream << "Failure: probing mate-31/mate-30 of " << currentAddress << " with ";
                logStream << "subnet TTL - 1 did result neither in a Time exceeded, neither in ";
                logStream << "an ICMP Echo reply, neither in an anonymous reply." << endl;
            }
            delete newRecord;
            newRecord = NULL;
        }
    }
    return success;
}


bool SubnetInferrer::discoverAlias_Rule_2_B(const ProbeRecord *siteRecord, 
                                            const InetAddress &currentAddress, 
                                            unsigned char currentSubnetPrefix)
{
    if(verbose)
    {
        logStream << "[Alias discovery rule n°2] Is " << currentAddress;
        logStream << " mate-31 or mate-30 of " << siteRecord->getDstAddress() << " ?" << endl;
    }

    bool success = false;
    if(currentSubnetPrefix == 31)
    {
        if(siteRecord->getDstAddress().is31Mate(currentAddress))
        {
            if(verbose)
            {
                logStream << "Success: " << currentAddress << " is the mate-31 of ";
                logStream << siteRecord->getDstAddress() << "." << endl;
            }
            success = true;
        }
        else if(verbose)
        {
            logStream << "Failure: while the current subnet is /31, " << currentAddress;
            logStream << " is not mate-31 of " << siteRecord->getDstAddress() << "." << endl;
        }
    }
    else if(currentSubnetPrefix == 30)
    {
        try
        {
            if(siteRecord->getDstAddress().is30Mate(currentAddress))
            {
                if(verbose)
                {
                    logStream << "Success: " << currentAddress << " is the mate-30 of ";
                    logStream << siteRecord->getDstAddress() << "." << endl;
                }
                success = true;
            }
            else if(verbose)
            {
                logStream << "Failure: while the current subnet is /30, " << currentAddress;
                logStream << " is not mate-30 of " << siteRecord->getDstAddress() << "." << endl;
            }
        }
        catch (InetAddressException &e)
        {
            // Does nothing, siteRecord->getDstAddress() can NOT have a /30mate because it is a broadcast/network address.
            if(verbose)
            {
                logStream << "Failure: " << siteRecord->getDstAddress() << " cannot have a ";
                logStream << "mate-30 because it is a broadcast/network address." << endl;
            }
        }
    }
    else if(verbose)
    {
        logStream << "Failure: this rule is only applicable for /31 and /30 subnets." << endl;
    }
    
    return success;
}

bool SubnetInferrer::discoverAlias_Rule_3_B(const ProbeRecord *sitePrevRecord,
                                            const InetAddress &currentAddress,
                                            unsigned char currentSubnetPrefix,
                                            unsigned char subnetdTTL) throw (SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Alias discovery rule n°3] Comparing reply at TTL - 1 for ";
        logStream << "mate-31/mate-30 of " << currentAddress << " with reply at same TTL ";
        logStream << "for " << sitePrevRecord->getDstAddress() << "..." << endl;
    }

    bool success = false;
    InetAddress mate31 = currentAddress.get31Mate();

    ProbeRecord *record = probe(mate31, subnetdTTL);
    if(record->isAnonymousRecord() || (!record->isAnonymousRecord() && 
       record->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
       (record->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNREACHABLE || 
        record->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNKNOWN)))
    {
        delete record;
        InetAddress mate30;
        try
        {
            mate30 = currentAddress.get30Mate();
        }
        catch(InetAddressException &e)
        {
            // This IP address cannot have a /30 mate
            mate30 = currentAddress.get31Mate().get30Mate();
        }
        record = probe(mate30, subnetdTTL);
    }

    if(!record->isAnonymousRecord())
    {
        if(record->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
        {
            if((record->getRplyAddress() == sitePrevRecord->getRplyAddress()))
            {
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Success: replies indeed originated from the same IP." << endl;
                }
                success = true;
            }
            else
            {
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Failure: replies come from distinct IPs." << endl;
                }
            }
        }
        else if(record->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            if(verbose)
            {
                if(debug)
                    logStream << endl;
                logStream << "Failure: mate-31/mate-30 of " << currentAddress;
                logStream << " is located closer in TTL, therefore on another subnet." << endl;
            }
            delete record;
            throw SubnetBarrierException(currentSubnetPrefix + 1);
        }
        else if(verbose)
        {
            if(debug)
                logStream << endl;
            logStream << "Failure: reply for " << currentAddress << " is neither Time exceeded, ";
            logStream << "neither ICMP Echo reply." << endl;
        }
    }
    else if(verbose)
    {
        if(debug)
            logStream << endl;
        logStream << "Failure: got an anonymous record after probing " << currentAddress << "." << endl;
    }

    delete record;

    return success;
}

bool SubnetInferrer::discoverAlias_Rule_4_B(const ProbeRecord *sitePrevRecord,
                                            const ProbeRecord *sitePrevPrevRecord,
                                            const InetAddress &currentAddress,
                                            unsigned char currentSubnetPrefix,
                                            unsigned char subnetdTTL) throw (SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Alias discovery rule n°4] Comparing replies at TTL - 2";
        if(sitePrevPrevRecord != NULL)
        {
            logStream << " for ";
            logStream << sitePrevPrevRecord->getDstAddress() << " and " << currentAddress;
        }
        logStream << "..." << endl;
    }

    bool success = false;
    ProbeRecord *newPrevPrevRecord = NULL;
    // Performs distance based test (which is applicable only for dTTL >= 2, hence record->getReqTTL >= 3)
    if(subnetdTTL >= 2 && sitePrevPrevRecord != NULL)
    {
        newPrevPrevRecord = probe(currentAddress, subnetdTTL - 1);
        // Distance based test where newPrevRecord is anonymous
        if(newPrevPrevRecord->isAnonymousRecord())
        {
            if(sitePrevPrevRecord->isAnonymousRecord())
            {
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Success: both replies are anonymous." << endl;
                }
                success = true;
            }
            else if(verbose)
            {
                if(debug)
                    logStream << endl;
                logStream << "Failure: reply when targetting " << sitePrevPrevRecord->getDstAddress() << " is anonymous." << endl;
            }
        }
        // Start of first else
        else
        {
            if(newPrevPrevRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(newPrevPrevRecord->getRplyAddress() == sitePrevPrevRecord->getRplyAddress())
                {
                    if(verbose)
                    {
                        if(debug)
                            logStream << endl;
                        logStream << "Success: both replies originated from the same IP." << endl;
                    }
                    success = true;
                }
                else
                {
                    /**
                     * If we get a time exceeded from a different ip address except the 
                     * sitePrevPrevRecord or tracePrevPrevRecord either the load balancing is 
                     * sending the packets on a third path or the probed ip is on the dTTL orbit 
                     * but far from our site. We assume the second is more probable because we 
                     * collected prevPrevRecords two times one populating site records and one 
                     * while tracing to the destination.
                     */
                    
                    if(verbose)
                    {
                        if(debug)
                            logStream << endl;
                        logStream << "Failure: replies come from a different IP, therefore targets are in different subnets." << endl;
                    }
                    delete newPrevPrevRecord;
                    newPrevPrevRecord = NULL;
                    throw SubnetBarrierException(currentSubnetPrefix + 1);

                }
            }
            else if(newPrevPrevRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                /**
                 * This IP is located closer to the vantage then the site itself
                 * so if this IP is in /x of siteRecord then both alias discovery and
                 * subnet growing must NOT go beyond /x+1 (shrinking subnet is equal to increasing prefix).
                 */
                
                if(verbose)
                {
                    if(debug)
                        logStream << endl;
                    logStream << "Failure: " << currentAddress << " is located closer, therefore on a different subnet." << endl;
                }
                
                delete newPrevPrevRecord;
                newPrevPrevRecord = NULL;
                
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
            else if(verbose)
            {
                if(debug)
                    logStream << endl;
                logStream << "Failure: reply from " << currentAddress << " is neither Time ";
                logStream << "exceeded, neither ICMP Echo reply." << endl;
            }
        }
        // End of first else
        delete newPrevPrevRecord;
        newPrevPrevRecord = NULL;
    }
    else if(verbose)
    {
        if(sitePrevPrevRecord == NULL)
        {
            if(debug)
                logStream << endl;
            logStream << "Failure: this rule is only applicable if there is a record for subnet TTL - 2." << endl;
        }
        else
        {
            if(debug)
                logStream << endl;
            logStream << "Failure: this rule is only applicable for targets at a distance of minimum 3 hops." << endl;
        }
    }
    return success;
}


bool SubnetInferrer::growSubnet_Rule_1_B(const ProbeRecord *newRecorddTTLPlus1, 
                                         unsigned char currentSubnetPrefix) throw(SubnetBarrierException)
{
    if(verbose)
    {
        logStream << "[Subnet growth rule n°1] Is " << newRecorddTTLPlus1->getDstAddress();
        logStream << " at reach with subnet TTL ?" << endl;
    }

    bool success = false;
    if(!newRecorddTTLPlus1->isAnonymousRecord())
    {
        if(newRecorddTTLPlus1->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            if(verbose)
            {
                logStream << "Success: " << newRecorddTTLPlus1->getDstAddress();
                logStream << " is reachable within the subnet TTL." << endl;
            }
            success = true;
        }
        else if(newRecorddTTLPlus1->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
        {
            if(verbose)
            {
                logStream << "Failure: " << newRecorddTTLPlus1->getDstAddress();
                logStream << " is not reachable with subnet TTL." << endl;
            }
            throw SubnetBarrierException(currentSubnetPrefix + 1);
        }
        else if(verbose)
        {
            logStream << "Failure: reply after probing " << newRecorddTTLPlus1->getDstAddress();
            logStream << " is neither Time exceeded neither ICMP Echo reply." << endl;
        }
    }
    else if(verbose)
    {
        logStream << "Failure: got an anonymous record after probing ";
        logStream << newRecorddTTLPlus1->getDstAddress() << "." << endl;
    }
    return success;
}

SubnetSite *SubnetInferrer::inferLocalAreaSubnet(const InetAddress &destinationAddress, NetworkAddress &localAreaNetwork)
{
    temporaryProbingCost = 0;
    unsigned char localSubnetPrefixLength = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(this->env->getLocalIPAddress());
    TimeVal currentTimeout = prober->getTimeout();
    prober->setTimeout(TimeVal(0,100000));
    TimeVal currentProbeRegulatingPausePeriod = prober->getProbeRegulatingPausePeriod();
    prober->setProbeRegulatingPausePeriod(TimeVal(0,0));
    SubnetSite *ss = new SubnetSite();
    ss->targetIPaddress = destinationAddress;
    ss->pivotIPaddress = destinationAddress;
    InetAddress firstAddress = localAreaNetwork.getLowerBorderAddress();
    firstAddress++; // Linux kernel does not allow you to probe local network address
    InetAddress lastAddress = localAreaNetwork.getUpperBorderAddress();
    lastAddress--; // Linux kernel does not allow you to probe local broadcast address
    ProbeRecord *pr;
    for(InetAddress currentAddress=firstAddress;currentAddress<=lastAddress;currentAddress++)
    {
        if(currentAddress == this->env->getLocalIPAddress())
            ss->insert(new SubnetSiteNode(currentAddress, localSubnetPrefixLength, 1, SubnetSiteNode::UNKNOWN_ALIAS_SX));
        else
        {
            pr = probe(currentAddress, 255);
            if(pr->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                ss->insert(new SubnetSiteNode(currentAddress, localSubnetPrefixLength, 1, SubnetSiteNode::UNKNOWN_ALIAS_SX));

            delete pr; 
            pr = NULL;
        }
    }
    ss->adjustLocalAreaSubnet(localAreaNetwork);
    ss->siCost = this->temporaryProbingCost;
    temporaryProbingCost = 0;
    cache.clear();

    prober->setTimeout(currentTimeout);
    prober->setProbeRegulatingPausePeriod(currentProbeRegulatingPausePeriod);

    return ss;
}

SubnetSite *SubnetInferrer::inferDummyLocalAreaSubnet(const InetAddress &destinationAddress, NetworkAddress &localAreaNetwork)
{
    SubnetSite *ss = new SubnetSite();
    ss->targetIPaddress = destinationAddress;
    ss->pivotIPaddress = destinationAddress;
    ss->adjustLocalAreaSubnet(localAreaNetwork);
    ss->siCost = 0;
    ss->spCost = 0;

    return ss;
}

ProbeRecord *SubnetInferrer::probe(const InetAddress &dst, unsigned char TTL)
{
    // Parameters obtained through env
    InetAddress localIPAddress = env->getLocalIPAddress();
    bool doubleProbe = env->usingDoubleProbe();
    bool useFixedFlowID = env->usingFixedFlowID();

    ProbeRecord *record = cache.fakeProbe(dst, TTL, useFixedFlowID);

    if(record == 0)
    {
        // Changes timeout if necessary for this IP
        IPLookUpTable *table = env->getIPTable();
        IPTableEntry *dstEntry = table->lookUp(dst);
        TimeVal preferredTimeout, currentTimeout;
        currentTimeout = prober->getTimeout();
        if(dstEntry != NULL)
            preferredTimeout = dstEntry->getPreferredTimeout();
        
        bool timeoutChanged = false;
        if(preferredTimeout > currentTimeout)
        {
            timeoutChanged = true;
            prober->setTimeout(preferredTimeout);
        }
    
        // Performs the probe
        if(doubleProbe == true)
            record = prober->doubleProbe(localIPAddress, dst, TTL, useFixedFlowID);
        else
            record = prober->singleProbe(localIPAddress, dst, TTL, useFixedFlowID);
        
        // Debug log for probes
        if(debug)
        {
            logStream << prober->getAndClearLog();
        }

        // Restores previous timeout value if it was changed
        if(timeoutChanged)
        {
            prober->setTimeout(currentTimeout);
        }

        temporaryProbingCost += record->getProbingCost();

        cache.insertProbeClone(record);
    }
    else
    {
        if(debug)
        {
            logStream << "\n[CACHED]" << record->toString();
        }
    }

    return record;
}
