/*
 * IPIDTuple.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in IPIDTuple.h (see this file to learn further about its goals).
 */

#include "IPIDTuple.h"

IPIDTuple::IPIDTuple(unsigned long int token, 
                     unsigned short ID, 
                     timeval time, 
                     bool e, 
                     unsigned char rTTL):
probeToken(token), 
IPID(ID), 
timeValue(time), 
echo(e), 
replyTTL(rTTL)
{
}

IPIDTuple::~IPIDTuple()
{
}

bool IPIDTuple::compare(IPIDTuple *tuple1, IPIDTuple *tuple2)
{
    if (tuple1->probeToken < tuple2->probeToken)
        return true;
    return false;
}
