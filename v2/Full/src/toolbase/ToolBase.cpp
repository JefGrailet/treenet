/*
 * ToolBase.cpp
 *
 *  Created on: Jul 16, 2012
 *      Author: engin
 *
 * Slightly edited in late december 2014 by J.-F. Grailet to harmonize coding style.
 */

#include "ToolBase.h"

TimeVal ToolBase::DEFAULT_PROBE_REGULATING_PAUSE_PERIOD(0, 100000);
unsigned int ToolBase::DEFAULT_MAX_CONSECUTIVE_ANONYMOUS = 4;
TimeVal ToolBase::DEFAULT_PROBE_TIMEOUT_PERIOD(2, 0);
unsigned int ToolBase::CONJECTURED_GLOBAL_INTERNET_DIAMETER = 64;
unsigned char ToolBase::MIN_CORE_IP_SUBNET_PREFIX = 20;
unsigned char ToolBase::MIN_CORE_IP_ALIAS_PREFIX = 28;
unsigned char ToolBase::DEFAULT_SUBNET_INFERENCE_MIDDLE_TTL = 12;
bool ToolBase::DEFAULT_DEBUG = false;
unsigned short ToolBase::ALIAS_RESO_DEFAULT_NB_IP_IDS = 4;
unsigned short ToolBase::ALIAS_RESO_DEFAULT_MAX_ROLLOVERS = 5;
double ToolBase::ALIAS_RESO_DEFAULT_BASE_TOLERANCE = 5.0;
double ToolBase::ALIAS_RESO_DEFAULT_MAX_ERROR = 0.3;

ToolBase::ToolBase(bool dbg):
debug(dbg)
{
}

ToolBase::~ToolBase() {}
