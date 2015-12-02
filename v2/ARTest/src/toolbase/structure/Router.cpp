/*
 * Router.cpp
 *
 *  Created on: Nov 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in Router.h (see this file to learn further about the goals of 
 * such class).
 */

#include <iostream>
using std::ostream;
#include <cmath>

#include "Router.h"

Router::Router()
{
}

Router::~Router()
{
}

void Router::addExpectedIP(InetAddress ia)
{
    expected.push_back(ia);
    expected.sort(InetAddress::smaller);
}

void Router::addUnresponsiveIP(InetAddress ia)
{
    unresponsive.push_back(ia);
    unresponsive.sort(InetAddress::smaller);
}

void Router::addInferredRouter(InferredRouter ir)
{
    inferredRouters.push_back(ir);
}

unsigned short Router::getNbExpectedIPs()
{
    return (unsigned short) expected.size();
}

unsigned short Router::getNbUnresponsiveIPs()
{
    return (unsigned short) unresponsive.size();
}

unsigned short Router::getNbInferredRouters()
{
    return (unsigned short) inferredRouters.size();
}

bool Router::canBeInferred()
{
    if(expected.size() == unresponsive.size())
    {
        return false;
    }
    return true;
}

bool Router::correctlyInferred()
{
    // There should be only one router
    if(inferredRouters.size() != 1)
    {
        return false;
    }
    
    // Computes responsive IPs (no need for a double loop since due to having sorted both lists)
    list<InetAddress> responsive;
    if(unresponsive.size() > 0)
    {
        list<InetAddress>::iterator j = unresponsive.begin();
        for(list<InetAddress>::iterator i = expected.begin(); i != expected.end(); ++i)
        {
            if((*i) == (*j))
                j++;
            else
                responsive.push_back((*i));
        }
    }
    else
    {
        for(list<InetAddress>::iterator i = expected.begin(); i != expected.end(); ++i)
            responsive.push_back((*i));
    }
    
    // Compares responsive IPs with the IPs in the (single) inferred router
    list<RouterInterface> *inferred = inferredRouters.front().getInterfacesList();
    list<RouterInterface>::iterator k = inferred->begin();
    for(list<InetAddress>::iterator i = responsive.begin(); i != responsive.end(); ++i)
    {
        if(k != inferred->end() && (*i) == (*k).ip)
            k++;
        else
            return false;
    }
    return true;
}

double Router::getSuccessRatio()
{
    unsigned short expectedSize = (unsigned short) expected.size();
    expectedSize -= (unsigned short) unresponsive.size();
    if(expectedSize == 0)
        return -1.0; // Tells it could not be inferred

    // Gets the size of the largest inferred router
    unsigned short largestSize = 0;
    for(list<InferredRouter>::iterator i = inferredRouters.begin(); 
                                           i != inferredRouters.end(); 
                                           ++i)
    {
        unsigned short curSize = (*i).getNbInterfaces();
        if(curSize > largestSize)
        {
            largestSize = curSize;
        }
    }
    
    // Special cases
    if(largestSize == 1 && expectedSize > 1)
    {
        return 0.0;
    }
    else if(largestSize == 1 && expectedSize == 1)
    {
        return 100.0;
    }
    
    double ratio = ((double) largestSize / (double) expectedSize) * 100;
    ratio = round(ratio * 100) / 100;
    return ratio;
}

string Router::toString()
{
    stringstream ss1;
    bool fullyUnresponsive = false;
    if(!this->canBeInferred())
        fullyUnresponsive = true;
    
    // Writes expected IPs (unresponsive included)
    bool first = true;
    list<InetAddress> responsive;
    
    if(unresponsive.size() > 0)
    {
        list<InetAddress>::iterator j = unresponsive.begin();
        for(list<InetAddress>::iterator i = expected.begin(); i != expected.end(); ++i)
        {
            if(first)
                first = false;
            else
                ss1 << " ";
        
            if((*i) == (*j))
            {
                j++;
                ss1 << (*i);
                if(!fullyUnresponsive)
                    ss1 << " [X]";
            }
            else
            {
                responsive.push_back((*i));
                ss1 << (*i);
            }
        }
    }
    else
    {
        for(list<InetAddress>::iterator i = expected.begin(); i != expected.end(); ++i)
        {
            if(first)
                first = false;
            else
                ss1 << " ";
            ss1 << (*i);
            
            responsive.push_back((*i));
        }
    }
    
    if(fullyUnresponsive)
    {
        stringstream ss2;
        ss2 << "IMPOSSIBLE - " << ss1.str() << "\n";
        return ss2.str();
    }
    
    // In case
    if(inferredRouters.size() == 0)
    {
        stringstream ss2;
        ss2 << "NOT CHECKED - " << ss1.str() << "\n";
        return ss2.str();
    }
    
    // Checks if the expected router was correctly inferred
    bool correctlyInferred = true;
    double successRate = this->getSuccessRatio();
    list<RouterInterface> *inferred = inferredRouters.front().getInterfacesList();
    list<RouterInterface>::iterator k = inferred->begin();
    for(list<InetAddress>::iterator i = responsive.begin(); i != responsive.end(); ++i)
    {
        if(k != inferred->end() && (*i) == (*k).ip)
            k++;
        else
        {
            correctlyInferred = false;
            break;
        }
    }
    
    stringstream ss2;
    if(correctlyInferred)
    {
        ss2 << "SUCCESS AT 100% - " << ss1.str() << "\n";
    }
    else if(successRate == 0.0)
    {
        ss2 << "FAIL - " << ss1.str() << "\n";
    }
    else
    {
        ss2 << "SUCCESS AT " << successRate << "% - " << ss1.str() << " : ";
        
        // Writes inferred routers
        bool firstRouter = true;
        for(list<InferredRouter>::iterator i = inferredRouters.begin(); 
                                           i != inferredRouters.end(); 
                                           ++i)
        {
            if(firstRouter)
                firstRouter = false;
            else
                ss2 << ", ";
        
            ss2 << "[";
            bool firstInterface = true;
            list<RouterInterface> *inferredI = (*i).getInterfacesList();
            for(list<RouterInterface>::iterator l = inferredI->begin(); l != inferredI->end(); ++l)
            {
                if(firstInterface)
                    firstInterface = false;
                else
                    ss2 << ", ";
                ss2 << (*l).ip;
                
                switch((*l).aliasMethod)
                {
                    case RouterInterface::IPID_VELOCITY:
                        ss2 << " (Speed)";
                        break;
                
                    case RouterInterface::ALLY:
                        ss2 << " (Ally)";
                        break;
                    
                    default:
                        break;
                }
            }
            ss2 << "]";
        }
        
        ss2 << "\n";
    }
    
    return ss2.str();
}
