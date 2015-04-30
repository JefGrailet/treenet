/*
 * ProbeRecordCache.h
 *
 *  Created on: Oct 1, 2010
 *      Author: engin
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#ifndef PROBERECORDCACHE_H_
#define PROBERECORDCACHE_H_

#include <deque>

#include "../../prober/structure/ProbeRecord.h"

class ProbeRecordCache
{
private:
	static const unsigned int MAX_PROBE_RECORD_CACHE_SIZE;
	
public:
	ProbeRecordCache();
	virtual ~ProbeRecordCache();
	
	bool insertProbeClone(const ProbeRecord *record);
	ProbeRecord *fakeProbe(const InetAddress &dstIP, unsigned char reqTTL, bool usingFixedFlowID);
	
	inline void clear() { probeRecorDQ.clear(); }
	
private:
	std::deque<ProbeRecord> probeRecorDQ;
};

#endif /* PROBERECORDCACHE_H_ */
