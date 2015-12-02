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

const bool SubnetInferrer::DEFAULT_STRICT_POSITIONING = false;
const unsigned char SubnetInferrer::DEFAULT_MIDDLE_TTL = 12;

SubnetInferrer::SubnetInferrer(TreeNETEnvironment *e, 
                               unsigned short li, 
                               unsigned short ui, 
                               unsigned short ls, 
                               unsigned short us) throw (SocketException):
env(e)
{
    debug = env->debugMode();
    srand(time(0));
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
}

SubnetInferrer::~SubnetInferrer()
{
    delete prober;
}

SubnetSite *SubnetInferrer::inferRemoteSubnet(const InetAddress &destinationAddress, 
                                              bool strictSubnetPositioning, 
                                              unsigned char middleTTL, 
                                              bool useLowerBorderAsWell) throw(UnresponsiveIPException, 
                                                                               UndesignatedPivotInterface, 
                                                                               ShortTTLException)
{
    SubnetSite *subnetSite = 0;
    if(destinationAddress.isUnset())
    {
        throw InetAddressException("Unrecognizable target IP Address");
    }

    if(debug)
    {
        cout << "Exploring the subnet accommodating the target IP address\"" << destinationAddress << "\"" << endl;
    }

    ProbeRecord *sitePrevPrevRecord = 0;
    ProbeRecord *sitePrevRecord = 0;
    ProbeRecord *siteRecord = 0;
    try
    {
        bool recordPopulation = populateRecords(destinationAddress, 
                                                &siteRecord, 
                                                &sitePrevRecord, 
                                                &sitePrevPrevRecord, 
                                                strictSubnetPositioning, 
                                                middleTTL, 
                                                useLowerBorderAsWell);
        
        int subnetPositioningProbingCost = temporaryProbingCost;
        temporaryProbingCost = 0;
        if(recordPopulation)
        {
            subnetSite = exploreSite_B(destinationAddress, siteRecord, sitePrevRecord, sitePrevPrevRecord, useLowerBorderAsWell);
            subnetSite->spCost = subnetPositioningProbingCost;
        }
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
                                     bool useLowerBorderAsWell) throw (UnresponsiveIPException, UndesignatedPivotInterface, ShortTTLException)
{
    bool success = false;
    (*siteRecord) = 0;
    (*sitePrevRecord) = 0;
    (*sitePrevPrevRecord) = 0;
    ProbeRecord *firstICMPEchoReply = 0;
    ProbeRecord *rec = 0;
    unsigned char TTL = middleTTL;
    const int MAX_CONSECUTIVE_ANONYMOUS_COUNT = 3;
    int consecutiveAnonymousCount = 0;
    std::set<InetAddress> appearingIPset;
    unsigned char dstTTL = 0;
    
    /**
     * (for dstTTL) Introduced long after coding this function to fill destinationIPdistance 
     * field of UndesignatedPivotInterface. It does not have any other effect.
     */

    if(debug)
    {
        cout << "  Populating Records" << endl;
        cout << "\tForward Probing" << endl;
    }
    
    /**
     * Sends probes to dst with an increasing TTL (by default, TTL starts at 1), with the hope
     * of discovering the hop distance between the vantage point and dst. This step stops when
     * -we crossed too many consecutive anonymous destinations (i.e. their ICMP reply did not 
     *  contain source); this amount is given by a constant
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
                firstICMPEchoReply = 0;
                delete rec;
                rec = 0;
                break;
            }
            consecutiveAnonymousCount = 0;
            if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                firstICMPEchoReply = rec;
                rec = 0;
                break;
            }
            else if(rec->getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                stop = true;
        }
        delete rec;
        rec = 0;
        TTL++;
    }
    while(!stop && consecutiveAnonymousCount < MAX_CONSECUTIVE_ANONYMOUS_COUNT && firstICMPEchoReply == 0 
          && TTL <= (unsigned char) ToolBase::CONJECTURED_GLOBAL_INTERNET_DIAMETER);

    /**
     * Next part is backward probing. This steps aims at correcting the TTL value if the TTL
     * computed at previous step is equal to the middleTTL (minimum TTL; default is 1). Indeed,
     * it is possible that the ICMP reply came from a device reachable with a TTL smaller than
     * middleTTL.
     */

    if((unsigned short) TTL > 1 && TTL == middleTTL && firstICMPEchoReply != 0)
    {
        if(debug)
        {
            cout << "\tBackward Probing" << endl;
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
                    rec = 0;
                }
                else
                {
                    delete rec;
                    rec = 0;
                    break;
                }
            }
            else
            {
                delete rec;
                rec = 0;
                break;
            }
            TTL--;
        }
    }
    
    // Registers the TTL for this IP
    IPTableEntry *IPEntry = env->getIPTable()->lookUp(dst);
    if(IPEntry != NULL)
    {
        IPEntry->setTTL(TTL);
    }
    
    // Stops probing if the program failed in obtaining an echo reply.
    if(firstICMPEchoReply == 0)
    {
        delete rec;
        std::string msg = "Target IP address \"" + *(dst.getHumanReadableRepresentation());
        msg += "\" is not responsive";
        throw UnresponsiveIPException(dst, msg);
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
    if(debug)
    {
        cout << "\tAnalyzing (probing /31 or /30 mate of siteRecord candidate) whether"
        << " siteRecord re-positioning is necessary)" << endl;
    }
    InetAddress mate31 = firstICMPEchoReply->getDstAddress().get31Mate(); // rplyAddress might be different from probed address (dstAddress)
    rec = probe(mate31, firstICMPEchoReply->getReqTTL()); // Checks mate-31
    
    // Gets mate-30 if mate-31 is anonymous, unknown or unreachable
    if(rec->isAnonymousRecord() ||
       (!rec->isAnonymousRecord() && rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
       (rec->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNREACHABLE || rec->getRplyICMPcode() == DirectProber::ICMP_CODE_HOST_UNKNOWN)))
    {
        delete rec;
        rec = 0;
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
        if(debug)
        {
            cout << "\tTrying to discover a new eggress (pivot) IP address" << endl;
        }
        
        unsigned char maxSearchPrefix = 30;
        if(!strictSubnetPositioning)
            maxSearchPrefix = 29;
    
        unsigned char currentSearchPrefix = 31;
        InetAddress firstIP, lastIP, currentIP;
        bool stopSearch = false;
        delete rec;
        rec = 0;

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
            
            if(debug)
            {
                cout << "\t\tTemp Network to search " << tmp << " firstIP:" << firstIP << " lastIP:" << lastIP << endl;
            }
            
            IPLookUpTable *table = env->getIPTable();
            for(currentIP = firstIP; currentIP <= lastIP; currentIP++)
            {
                if(debug)
                {
                    cout << "\t\tCurrentIP:" << currentIP << endl;
                }
                
                // New (Oct 8, 2015): skips IP which are known to be unresponsive
                if(table->lookUp(currentIP) == NULL)
                {
                    continue;
                }
                
                // If currentIP is not in the set, probe it (TTL+1) and check results
                if(appearingIPset.find(currentIP) == appearingIPset.end())
                {
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
                                rec = 0;
                                break;
                            }
                            // Additionnal probes
                            else
                            {
                                ProbeRecord *tmpRec = 0;
                                tmpRec = probe(currentIP, firstICMPEchoReply->getReqTTL());
                                // currentIP is at same distance; it is not on the subnet: we stop
                                if(!tmpRec->isAnonymousRecord() && tmpRec->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                                {
                                    delete rec;
                                    rec = 0;
                                    delete tmpRec;
                                    tmpRec = 0;
                                    stopSearch = true;
                                    break;
                                }
                                delete tmpRec;
                                tmpRec = 0;
                                tmpRec = probe(currentIP.get31Mate(), firstICMPEchoReply->getReqTTL() + 1);
                                // 31-mate of currentIP is out of range; it is not on the subnet
                                if(!tmpRec->isAnonymousRecord() && tmpRec->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                                {
                                    delete rec;
                                    rec = 0;
                                    delete tmpRec; 
                                    tmpRec = 0;
                                    stopSearch = true;
                                    break;
                                }
                                delete tmpRec;
                                tmpRec = 0;
                                
                                // currentIP is a valid pivot
                                (*siteRecord) = rec;
                                (*siteRecord)->setRplyAddress(currentIP); // In case currentIP differs from (*siteRecord)->getDstAddress()
                                rec = 0;
                                break;
                            }
                        }
                        // Stops with current prefix if we exceeded TTL (actually TTL+1)
                        else if(rec->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
                        {
                            delete rec;
                            rec = 0;
                            stopSearch = true;
                            break;
                        }
                    }
                    
                    // Since currentIP is an anonymous node, deletes rec
                    delete rec;
                    rec = 0;
                }
            }
            currentSearchPrefix--;
        }
    }
    // We do not need to re-position the site
    else
    {
        if(debug)
        {
            cout << "\t  No need to re-position siteRecord" << endl;
        }
        (*siteRecord) = firstICMPEchoReply;
        (*siteRecord)->setRplyAddress(firstICMPEchoReply->getDstAddress()); // If rplyAddress differs from dstAddress
        
        firstICMPEchoReply = 0;
    }
    delete firstICMPEchoReply; 
    firstICMPEchoReply = 0;
    
    if(rec != 0)
    {
        delete rec;
        rec = 0;
    }
    
    // Valid pivot
    if((*siteRecord) != 0 && !(*siteRecord)->isAnonymousRecord())
    {
        unsigned char siteRecordTTL = (*siteRecord)->getReqTTL();
        
        // Pivot is too close
        if(siteRecordTTL < 2)
        {
            delete (*siteRecord);
            (*siteRecord) = 0;
            
            std::string msg = "Pivot IP address \"" + *(dst.getHumanReadableRepresentation());
            msg += "\" should at least be located at 2 hops distance away from the vantage point";
            throw ShortTTLException(dst, msg);
        }
        // Success! Probes sitePrevRecord and sitePrevPrevRecord with TTL-1 and TTL-2
        else
        {
            success = true;
            if(debug)
            {
                cout << "\t" << "Attempting to figure out prevRecord and prevPrevRecord" << endl;
            }
            (*sitePrevRecord) = probe((*siteRecord)->getDstAddress(), siteRecordTTL - 1);
            if(siteRecordTTL > 2)
            {
                (*sitePrevPrevRecord) = probe((*siteRecord)->getDstAddress(), siteRecordTTL - 2);
            }
        }
    }
    // Anonymous pivot or no pivot at all: throws an exception
    else
    {
        if((*siteRecord) != 0)
        {
            delete (*siteRecord); 
            (*siteRecord) = 0;
        }
        
        std::string msg = "Undesignated pivot interface for the target site hosting IP address \"";
        msg += *(dst.getHumanReadableRepresentation())+"\"";
        throw UndesignatedPivotInterface(dst, dstTTL, msg);
    }

    if(debug)
    {
        cout << "\tPopulated Records - siteRecord:"
        << ((*siteRecord) == 0 ? "NULL" : *((*siteRecord)->getDstAddress().getHumanReadableRepresentation()))
        << " - sitePrevRecord:" << ((*sitePrevRecord) == 0 ? "NULL" : *((*sitePrevRecord)->getRplyAddress().getHumanReadableRepresentation()))
        << " - sitePrevPrevRecord:" 
        << ((*sitePrevPrevRecord) == 0 ? "NULL": *((*sitePrevPrevRecord)->getRplyAddress().getHumanReadableRepresentation())) << endl;
    }

    return success;
}

SubnetSite *SubnetInferrer::exploreSite_B(const InetAddress &dst,
                                          ProbeRecord *siteRecord,
                                          ProbeRecord *sitePrevRecord,
                                          ProbeRecord *sitePrevPrevRecord,
                                          bool useLowerBorderAsWell)
{
    /**
     * If the record is ANONYMOUS then we cannot evaluate /x of the IP belongs to record.
     * Hence, we can neither discover alias nor explore subnet.
     */
    
    if(siteRecord == 0 || siteRecord->isAnonymousRecord())
    {
        return 0;
    }

    /**
     * aliasRecord and alias objects refer to the contra-pivot interface in the paper.
     * We call them as alias here because contra-pivot interface and the sitePrevRecord 
     * IP addresses are hosted by the same router.
     */
    
    ProbeRecord *aliasRecord = 0;
    InetAddress alias; // By default it is undefined i.e. 0

    enum SubnetSiteNode::AliasDiscoveryMethod aliasDiscoveryMethod = SubnetSiteNode::UNKNOWN_ALIAS_SX;
    // Unsigned char aliasPrefix = 0;

    unsigned char subnetPrefix = 31;
    // contra-pivot distance from vantage point
    unsigned char subnetdTTL = siteRecord->getReqTTL()-1; // We use this instead of getting directly from sitePrevRecord because it might be NULL
    bool subnetMate3130QuotaFilled = false;
    bool aliasMate3130QuotaFilled = false;
    bool rule3, rule4, rule5;
    InetAddressSet probedIPset;
    ProbeRecord *newRecorddTTLPlus1 = 0;
    ProbeRecord *newRecorddTTL = 0;
    SubnetSite *subnetSite = new SubnetSite();
    subnetSite->targetIPaddress = dst;
    subnetSite->pivotIPaddress = siteRecord->getDstAddress();
    subnetSite->prevSiteIPaddress = sitePrevRecord->getRplyAddress();
    subnetSite->prevSiteIPaddressDistance = sitePrevRecord->getReqTTL();

    try
    {
        /**
         * The ip of the record is always the first member of the subnet so let us immediately 
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
        
        while(subnetPrefix >= ToolBase::MIN_CORE_IP_SUBNET_PREFIX)
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
                    if(debug)
                    {
                        cout << "\tStopping growing the subnet because there are "
                        << subnetSite->getTotalSize() << " IPs while it should has been "
                        << thresholdSubnetSize + 1 << " to move from /" << (int) subnetPrefix + 1 << " to "
                        << (int) subnetPrefix << endl;
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
                    if(debug)
                    {
                        cout << "\tStopping growing the subnet because prefix /" 
                        << (unsigned short) subnetPrefix + 1 << " encompasses LAN. /"
                        << (unsigned short) subnetPrefix << " will be kept." << endl;
                    }
                    break; // STOP growing subnet further
                }
            }

            // Second loop to check all addresses within the subnet
            IPLookUpTable *table = env->getIPTable();
            for(InetAddress currentAddress = firstAddress; currentAddress <= lastAddress; currentAddress++)
            {
                // Improvement (Oct 8, 2015): skips IPs known to be unresponsive
                if(table->lookUp(currentAddress) == NULL)
                {
                    continue;
                }
            
                if(!probedIPset.contains(currentAddress))
                {
                    probedIPset.insert(new InetAddress(currentAddress));
                    if(debug)
                    {
                        cout << "\t** Analyzing candidate " << currentAddress << endl;
                    }
                    newRecorddTTLPlus1 = probe(currentAddress, subnetdTTL + 1); // NEW PROBE
                    if(!newRecorddTTLPlus1->isAnonymousRecord())
                    {
                        // Apply grow subnet rule 1
                        if(growSubnet_Rule_1_B(newRecorddTTLPlus1, subnetPrefix))
                        {
                            /**
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

                            // START ALIAS DISCOVERY
                            
                            if(sitePrevRecord != 0 && !sitePrevRecord->isAnonymousRecord() && aliasRecord == 0)
                            {
                                if(debug)
                                {
                                    cout << "\tStarting Alias Discovery" << endl;
                                }
                                newRecorddTTL = probe(currentAddress, subnetdTTL); // NEW PROBE
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
                                        if(debug)
                                        {
                                            cout << "\t  " << currentAddress << " is ALIAS and on the subnet" << endl;
                                        }
                                        aliasRecord = new ProbeRecord(*newRecorddTTL);
                                        alias = currentAddress;
                                        // AliasPrefix = subnetPrefix;
                                        subnetSite->insert(new SubnetSiteNode(currentAddress, subnetPrefix, subnetdTTL, aliasDiscoveryMethod));
                                        
                                        delete newRecorddTTLPlus1;
                                        newRecorddTTLPlus1 = 0;
                                        delete newRecorddTTL;
                                        newRecorddTTL = 0;
                                        continue;
                                    }
                                }
                                
                                /**
                                 * End of if(!newRecorddTTL->isAnonymousRecord() && 
                                 * newRecorddTTL->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
                                 */
                                
                                if(debug)
                                {
                                    cout << "\tEnding Alias Discovery" << endl;
                                }
                            }
                            else
                            {
                                if(debug)
                                {
                                    cout << "\tSkipping Alias Discovery because either sitePrevRecord is anonymous or alias already found" << endl;
                                }
                            }

                            // END ALIAS DISCOVERY

                            if(subnetPrefix >= 30 && subnetMate3130QuotaFilled == false)
                            {
                                subnetMate3130QuotaFilled = growSubnet_Rule_2_B(siteRecord, currentAddress, subnetPrefix);
                                if(subnetMate3130QuotaFilled)
                                {
                                    if(debug)
                                    {
                                        cout << "\t" << currentAddress << " is on the subnet" << endl;
                                    }
                                    subnetSite->insert(new SubnetSiteNode(currentAddress, subnetPrefix, 
                                                                          subnetdTTL + 1, SubnetSiteNode::UNKNOWN_ALIAS_SX));
                                    
                                    delete newRecorddTTLPlus1;
                                    newRecorddTTLPlus1 = 0;
                                    delete newRecorddTTL;
                                    newRecorddTTL = 0;
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
                                        if(debug)
                                        {
                                            cout << "\t" << currentAddress << " is on the subnet" << endl;
                                        }
                                        subnetSite->insert(new SubnetSiteNode(currentAddress, 
                                                                              subnetPrefix, 
                                                                              subnetdTTL+1, 
                                                                              SubnetSiteNode::UNKNOWN_ALIAS_SX));
                                    }
                                }
                            }
                            delete newRecorddTTL;
                            newRecorddTTL = 0;
                        }
                        // End of if(growSubnet_Rule_1_B(newRecord,subnetPrefix))
                    }
                    delete newRecorddTTLPlus1; 
                    newRecorddTTLPlus1 = 0;
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
        newRecorddTTLPlus1 = 0;
        delete newRecorddTTL;
        newRecorddTTL = 0;
        
        subnetSite->markSubnetOvergrowthElements(e.barrierPrefix, SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
        if(debug)
        {
            cout << "Site Subnet after clearing out barrier" << endl;
            cout << (*subnetSite) << endl;
        }
    }

    probedIPset.hardReleaseMemory();

    subnetSite->adjustRemoteSubnet2(useLowerBorderAsWell);

    if(aliasRecord != 0)
    {
        delete aliasRecord;
        aliasRecord = 0;
    }
    return subnetSite;
}

bool SubnetInferrer::discoverAlias_Rule_1_B(const ProbeRecord *sitePrevRecord, const ProbeRecord *newRecorddTTL)
{
    bool success = false;
    if(newRecorddTTL->getRplyAddress() == sitePrevRecord->getRplyAddress())
    {
        if(debug)
        {
            cout << "\tAlias Discovery RULE-1 (source matching) is successful" << endl;
        }
        success = true;
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_2_B(const ProbeRecord *siteRecord,
                                         const InetAddress &currentAddress,
                                         unsigned char currentSubnetPrefix) throw(SubnetBarrierException)
{
    bool success = false;
    if(currentSubnetPrefix == 31)
    {
        if(siteRecord->getDstAddress().is31Mate(currentAddress))
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-2(/31Mate of the record) is successful" << endl;
            }
            success = true;
        }
    }
    else if(currentSubnetPrefix == 30)
    {
        try
        {
            if(siteRecord->getDstAddress().is30Mate(currentAddress))
            {
                if(debug)
                {
                    cout << "\tGrow Subnet RULE-2(/30Mate of the record) is successful" << endl;
                }
                success = true;
            }
        }
        catch (InetAddressException &e)
        {
            // Does nothing, siteRecord->getNodeAddress() can NOT have a /30mate because it is a broadcast/network address.
        }
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_3_B(const ProbeRecord *sitePrevRecord,
                                         const ProbeRecord *newRecorddTTL,
                                         unsigned char currentSubnetPrefix) throw(SubnetBarrierException)
{
    bool success = false;
    if(newRecorddTTL->isAnonymousRecord())
    {
        if(sitePrevRecord->isAnonymousRecord())
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-3(probe with dTTL and prevRecord are anonymous) passed" << endl;
            }
            success = true;
        }
    }
    else if(newRecorddTTL->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
    {
        if(!sitePrevRecord->isAnonymousRecord() && sitePrevRecord->getRplyAddress() == newRecorddTTL->getRplyAddress())
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-3(probe with dTTL and prevRecord share same ip) passed" << endl;
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
            
            if(debug)
            {
                cout << "\tGrow Subnet RULE-3(barrier discovered -dTTL probe returns"
                << " TIME_EXEEDED but via a different prevRecord-) failed" << endl;
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
        
        if(debug)
        {
            cout << "\tGrow Subnet RULE-3(barrier discovered -dTTL probe returns"
            << " neither anonymous nor TIME_EXCEEDED-) failed" << endl;
        }
        throw SubnetBarrierException(currentSubnetPrefix + 1);
    }
    return success;
}

bool SubnetInferrer::growSubnet_Rule_4_B(const InetAddress &currentAddress, 
                                         unsigned char currentSubnetPrefix, 
                                         unsigned char subnetdTTL) throw(SubnetBarrierException)
{
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
        if(debug)
        {
            cout << "\tGrow Subnet RULE-4(proing /31 mate with dTTL+1 does NOT return TIME_EXCEEDED) passed" << endl;
        }
        success = true;
    }
    else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
    {
        delete newRecord;
        newRecord = 0;
        if(debug)
        {
            cout << "\tGrow Subnet RULE-4(barrier discovered -probing /31 mate with dTTL+1 returns TIME_EXCEEDED-) failed" << endl;
        }
        throw SubnetBarrierException(currentSubnetPrefix + 1);
    }
    delete newRecord;
    newRecord = 0;
    
    return success;
}

bool SubnetInferrer::growSubnet_Rule_5_B(const InetAddress &currentAddress, 
                                         const InetAddress &alias, 
                                         unsigned char currentSubnetPrefix, 
                                         unsigned char subnetdTTL)throw(SubnetBarrierException)
{
    InetAddress mate31 = currentAddress.get31Mate();
    bool success = false;
    if(!alias.isUnset()) // We have a valid alias
    {
        if(mate31 == alias)
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-5(mate31 is equal to alias) passed" << endl;
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
                    mate30=currentAddress.get30Mate();
                }
                catch(InetAddressException &e)
                {
                    mate30=mate31.get30Mate();
                }
                newRecord=probe(mate30, subnetdTTL);
            }

            if(newRecord->isAnonymousRecord() || newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(debug)
                {
                    cout << "\tGrow Subnet RULE-5(alias=VALID IP, proing /31 mate with dTTL does NOT return ECHO_REPLY) passed" << endl;
                }
                success = true;
            }
            else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newRecord;
                newRecord = 0;
                if(debug)
                {
                    cout << "\tGrow Subnet RULE-5(alias=VALID IP, barrier discovered -proing"
                    << " /31 mate with dTTL returns ECHO REPLY-) failed" << endl;
                }
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
            delete newRecord;
            newRecord = 0;
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
        
        if(currentAddress<mate31)
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-5(alias=UNDEFINED, currentAddress<mate31) temporarily passed" << endl;
            }
            success = true;
        }
        else
        {
            ProbeRecord *newRecord = probe(mate31, subnetdTTL);
            if(newRecord->isAnonymousRecord() || newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(debug)
                {
                    cout << "\tGrow Subnet RULE-5(alias=UNDEFINED, proing /31 mate with dTTL does NOT return ECHO_REPLY) passed" << endl;
                }
                success = true;
            }
            else if(newRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                delete newRecord;
                newRecord = 0;
                if(debug)
                {
                    cout << "\tGrow Subnet RULE-5(alias=UNDEFINED, barrier discovered -proing"
                    << " /31 mate with dTTL returns ECHO REPLY-) failed" << endl;
                }
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
            delete newRecord;
            newRecord = 0;
        }
    }
    return success;
}


bool SubnetInferrer::discoverAlias_Rule_2_B(const ProbeRecord *siteRecord, 
                                            const InetAddress &currentAddress, 
                                            unsigned char currentSubnetPrefix)
{
    bool success = false;
    if(currentSubnetPrefix == 31)
    {
        if(siteRecord->getDstAddress().is31Mate(currentAddress))
        {
            if(debug)
            {
                cout << "\tAlias Discovery RULE-2(/31Mate of the record) is successful" << endl;
            }
            success = true;
        }
    }
    else if(currentSubnetPrefix == 30)
    {
        try
        {
            if(siteRecord->getDstAddress().is30Mate(currentAddress))
            {
                if(debug)
                {
                    cout << "\tAlias Discovery RULE-2(/30Mate of the record) is successful" << endl;
                }
                success = true;
            }
        }
        catch (InetAddressException &e)
        {
            // Does nothing, siteRecord->getNodeAddress() can NOT have a /30mate because it is a broadcast/network address.
        }
    }
    return success;
}

bool SubnetInferrer::discoverAlias_Rule_3_B(const ProbeRecord *sitePrevRecord,
                                            const InetAddress &currentAddress,
                                            unsigned char currentSubnetPrefix,
                                            unsigned char subnetdTTL) throw (SubnetBarrierException)
{
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
                if(debug)
                {
                    cout << "\tAlias Discovery RULE-3 (Palmtree matching-) is successful" << endl;
                }
                success = true;
            }
        }
        else if(record->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            if(debug)
            {
                cout << "\tAlias Discovery RULE-3 (subnet barrier discovered -closer subnet/frontier interface-) failed" << endl;
            }
            delete record;
            throw SubnetBarrierException(currentSubnetPrefix + 1);
        }
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
    bool success = false;
    ProbeRecord *newPrevPrevRecord = 0;
    // Performs distance based test (which is applicable only for dTTL >= 2, hence record->getReqTTL >= 3)
    if(subnetdTTL >= 2 && sitePrevPrevRecord != 0)
    {
        newPrevPrevRecord = probe(currentAddress, subnetdTTL - 1);
        // Distance based test where newPrevRecord is anonymous
        if(newPrevPrevRecord->isAnonymousRecord())
        {
            if(sitePrevPrevRecord->isAnonymousRecord())
            {
                if(debug)
                {
                    cout << "\tAlias Discovery RULE-4 (prevPrevRecord matching -both anonymous-) is successful" << endl;
                }
                success = true;
            }
        }
        // Start of first else
        else
        {
            if(newPrevPrevRecord->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                if(newPrevPrevRecord->getRplyAddress() == sitePrevPrevRecord->getRplyAddress())
                {
                    if(debug)
                    {
                        cout << "\tAlias Discovery RULE-4 (prevPrevRecord matching -both same ip-) is successful" << endl;
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
                    
                    if(debug)
                    {
                        cout << "\tAlias Discovery RULE-4 (subnet barrier discovered -same orbit different subnet-) failed" << endl;
                    }
                    delete newPrevPrevRecord;
                    newPrevPrevRecord = 0;
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
                
                if(debug)
                {
                    cout << "\tAlias Discovery RULE-4 (subnet barrier discovered -closer subnet-) failed" << endl;
                }
                
                delete newPrevPrevRecord;
                newPrevPrevRecord = 0;
                
                throw SubnetBarrierException(currentSubnetPrefix + 1);
            }
        }
        // End of first else
        delete newPrevPrevRecord;
        newPrevPrevRecord = 0;
    }
    return success;
}


bool SubnetInferrer::growSubnet_Rule_1_B(const ProbeRecord *newRecorddTTLPlus1, 
                                         unsigned char currentSubnetPrefix) throw(SubnetBarrierException)
{
    bool success = false;
    if(!newRecorddTTLPlus1->isAnonymousRecord())
    {
        if(newRecorddTTLPlus1->getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-1(probe with dTTL+1) is successful" << endl;
            }
            success = true;
        }
        else if(newRecorddTTLPlus1->getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
        {
            if(debug)
            {
                cout << "\tGrow Subnet RULE-1(barrier discovered -probe with dTTL+1 returns TIME_EXCEEDED-) failed" << endl;
            }
            throw SubnetBarrierException(currentSubnetPrefix + 1);
        }
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
            pr = 0;
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
            record = prober->doubleProbe(localIPAddress, dst, TTL, useFixedFlowID, false, 0);
        else
            record = prober->singleProbe(localIPAddress, dst, TTL, useFixedFlowID, false, 0);

        // Restores previous timeout value if it was changed
        if(timeoutChanged)
        {
            prober->setTimeout(currentTimeout);
        }

        temporaryProbingCost += record->getProbingCost();

        cache.insertProbeClone(record);

        if(debug)
        {
            cout << "\t\tProbe Result:" << *record << endl;
        }
    }
    else
    {
        if(debug)
        {
            cout << "\t\tProbe-CACHE Result:" << *record << endl;
        }
    }

    return record;
}
