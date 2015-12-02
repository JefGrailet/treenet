/*
 * ToolBase.h
 *
 *  Created on: Jul 16, 2012
 *      Author: engin
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 *
 * N.B. (Sep 2015): this class is, for now, exclusively used for classes which originate from 
 * ExploreNET. Parts introduced for TreeNET prints out in console by default (as a way to check 
 * progress) or have methods explicitely printing out results (such as display methods in the 
 * network tree classes) and do not rely on the constants displayed below.
 */

#ifndef TOOLBASE_H_
#define TOOLBASE_H_

#include "../common/date/TimeVal.h"

class ToolBase
{
public:
    enum ProbeTypeEnum{UDP, ICMP_ECHO_REQUEST};
    static TimeVal DEFAULT_PROBE_REGULATING_PAUSE_PERIOD;
    static unsigned int DEFAULT_MAX_CONSECUTIVE_ANONYMOUS;
    static TimeVal DEFAULT_PROBE_TIMEOUT_PERIOD;
    static unsigned int CONJECTURED_GLOBAL_INTERNET_DIAMETER;
    static unsigned char MIN_CORE_IP_SUBNET_PREFIX;
    static unsigned char MIN_CORE_IP_ALIAS_PREFIX;
    static unsigned char DEFAULT_SUBNET_INFERENCE_MIDDLE_TTL;
    static bool DEFAULT_DEBUG;

    ToolBase(bool verbose = ToolBase::DEFAULT_DEBUG);
    virtual ~ToolBase();

    inline void setDebug(bool dbg) { this->debug = dbg; }

protected:
    bool debug;
};

#endif /* TOOLBASE_H_ */
