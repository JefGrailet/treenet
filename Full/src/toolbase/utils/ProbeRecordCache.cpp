/*
 * ProbeRecordCache.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: engin
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#include "ProbeRecordCache.h"

const unsigned int ProbeRecordCache::MAX_PROBE_RECORD_CACHE_SIZE = 32;

ProbeRecordCache::ProbeRecordCache()
{
}

ProbeRecordCache::~ProbeRecordCache()
{
    probeRecorDQ.clear();
}

bool ProbeRecordCache::insertProbeClone(const ProbeRecord *record)
{
    if(probeRecorDQ.size() >= ProbeRecordCache::MAX_PROBE_RECORD_CACHE_SIZE)
    {
        probeRecorDQ.pop_front();
    }
    probeRecorDQ.push_back(*record);
    return true;
}

ProbeRecord *ProbeRecordCache::fakeProbe(const InetAddress &dstIP, unsigned char reqTTL, bool usingFixedFlowID)
{
    ProbeRecord *result = 0;
    std::deque<ProbeRecord>::const_reverse_iterator revIter = probeRecorDQ.rbegin();
    std::deque<ProbeRecord>::const_reverse_iterator revIterEnd = probeRecorDQ.rend();
    for(; revIter != revIterEnd; revIter++)
    {
        if(revIter->getDstAddress() == dstIP && revIter->getReqTTL() == reqTTL)
        {
            // If usingFixedFlowID is true, return only the response collected by usingFixedFlowID=true parameter
            if(usingFixedFlowID == true)
            {
                if(revIter->getUsingFixedFlowID() == true)
                {
                    result = new ProbeRecord(*revIter);
                    break;
                }
            }
            // Otherwise, return the response either collected by usingFixedFlowID=true or usingFixedFlowID=false parameter
            else
            {
                result = new ProbeRecord(*revIter);
                break;
            }
        }
    }
    return result;
}
