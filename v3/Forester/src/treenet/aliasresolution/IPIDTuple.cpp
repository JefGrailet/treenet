/*
 * IPIDTuple.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in IPIDTuple.h (see this file to learn further about its goals).
 */

#include "IPIDTuple.h"

IPIDTuple::IPIDTuple()
{
    target = InetAddress(0);
    probeToken = 0;
    IPID = 0;
    gettimeofday(&timeValue, NULL);
    echo = false;
    replyTTL = (unsigned char) 0;
}

IPIDTuple::IPIDTuple(InetAddress target)
{
    this->target = target;
    
    // Default values for other fields
    probeToken = 0;
    IPID = 0;
    gettimeofday(&timeValue, NULL);
    echo = false;
    replyTTL = (unsigned char) 0;
}

IPIDTuple::~IPIDTuple()
{
}

IPIDTuple &IPIDTuple::operator=(const IPIDTuple &other)
{
    this->target = other.target;
    this->probeToken = other.probeToken;
    this->IPID = other.IPID;
    this->timeValue = other.timeValue;
    this->echo = other.echo;
    this->replyTTL = other.replyTTL;
    return *this;
}

bool IPIDTuple::compareByTime(IPIDTuple &tuple1, IPIDTuple &tuple2)
{
    if (tuple1.timeValue.tv_sec < tuple2.timeValue.tv_sec)
        return false;
    else if(tuple1.timeValue.tv_usec < tuple2.timeValue.tv_usec)
        return false;
    return true;
}
