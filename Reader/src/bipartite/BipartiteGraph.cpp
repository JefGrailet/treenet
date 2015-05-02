/*
 * BipartiteGraph.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: grailet
 *
 * Implements the class defined in BipartiteGraph.h (see this file to learn further about the 
 * goals of such class).
 */
 
#include <sstream>
using std::stringstream;

#include "BipartiteGraph.h"

BipartiteGraph::BipartiteGraph()
{
    this->switchCounter = 1;
    this->routerCounter = 1;
    this->subnetCounter = 1;
}

BipartiteGraph::~BipartiteGraph()
{
    for(list<BipartiteSwitch*>::iterator i = switches.begin(); i != switches.end(); ++i)
    {
        delete (*i);
    }
    switches.clear();

    for(list<BipartiteRouter*>::iterator i = routers.begin(); i != routers.end(); ++i)
    {
        delete (*i);
    }
    routers.clear();
    
    for(list<BipartiteSubnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        delete (*i);
    }
    subnets.clear();
    
    for(list<LinkSwitchRouter*>::iterator i = linksSR.begin(); i != linksSR.end(); ++i)
    {
        delete (*i);
    }
    linksSR.clear();
    
    for(list<LinkRouterSubnet*>::iterator i = linksRS.begin(); i != linksRS.end(); ++i)
    {
        delete (*i);
    }
    linksRS.clear();
}

void BipartiteGraph::addRouter(Router *r)
{
    // Creates the label
    stringstream labelStream;
    labelStream << "R";
    labelStream << routerCounter;
    string label = labelStream.str();
    routerCounter++;
    
    // Creates the bipartite element
    BipartiteRouter *bipRouter = new BipartiteRouter(label, r);
    r->setBipEquivalent(bipRouter);
    routers.push_back(bipRouter);
}

void BipartiteGraph::addSubnet(SubnetSite *ss)
{
    // Creates the label
    stringstream labelStream;
    labelStream << "S";
    labelStream << subnetCounter;
    string label = labelStream.str();
    subnetCounter++;
    
    // Creates the bipartite element
    BipartiteSubnet *bipSubnet = new BipartiteSubnet(label, ss);
    ss->setBipEquivalent(bipSubnet);
    subnets.push_back(bipSubnet);
}

BipartiteSwitch *BipartiteGraph::createSwitch()
{
    // Creates the label
    stringstream labelStream;
    labelStream << "E";
    labelStream << switchCounter;
    string label = labelStream.str();
    switchCounter++;
    
    // Creates the bipartite element
    BipartiteSwitch *bipSwitch = new BipartiteSwitch(label);
    switches.push_back(bipSwitch);
    
    return bipSwitch;
}

BipartiteRouter *BipartiteGraph::createImaginaryRouter()
{
    // Creates the label
    stringstream labelStream;
    labelStream << "R";
    labelStream << routerCounter;
    string label = labelStream.str();
    routerCounter++;
    
    // Creates the bipartite element
    BipartiteRouter *bipRouter = new BipartiteRouter(label);
    routers.push_back(bipRouter);
    
    return bipRouter;
}

BipartiteSubnet *BipartiteGraph::createImaginarySubnet()
{
    // Creates the label
    stringstream labelStream;
    labelStream << "S";
    labelStream << subnetCounter;
    string label = labelStream.str();
    subnetCounter++;
    
    // Creates the bipartite element
    BipartiteSubnet *bipSubnet = new BipartiteSubnet(label);
    subnets.push_back(bipSubnet);
    
    return bipSubnet;
}

void BipartiteGraph::createLinkSR(BipartiteSwitch *bipSwitch, BipartiteRouter *bipRouter)
{
    if(bipSwitch == NULL || bipRouter == NULL)
        return;
    
    LinkSwitchRouter *link = new LinkSwitchRouter(bipSwitch, bipRouter);
    linksSR.push_back(link);
    
    bipSwitch->addLink(link);
    bipRouter->addLinkSwitch(link);
}

void BipartiteGraph::createLinkRS(BipartiteRouter *bipRouter, BipartiteSubnet *bipSubnet)
{
    if(bipRouter == NULL || bipSubnet == NULL)
        return;
    
    LinkRouterSubnet *link = new LinkRouterSubnet(bipRouter, bipSubnet);
    linksRS.push_back(link);
    
    bipRouter->addLinkSubnet(link);
    bipSubnet->addLink(link);
}

void BipartiteGraph::createLink(Router *r, SubnetSite *ss)
{
    BipartiteRouter *bipRouter = r->getBipEquivalent();
    BipartiteSubnet *bipSubnet = ss->getBipEquivalent();
    
    this->createLinkRS(bipRouter, bipSubnet);
}

void BipartiteGraph::removeLinkSR(string switchLabel, string routerLabel)
{
    for(list<LinkSwitchRouter*>::iterator i = linksSR.begin(); i != linksSR.end(); ++i)
    {
        string curSwitchLabel = (*i)->getSwitch()->getLabel();
        string curRouterLabel = (*i)->getRouter()->getLabel();
        
        if(switchLabel.compare(curSwitchLabel) == 0 && routerLabel.compare(curRouterLabel) == 0)
        {
            linksSR.erase(i--);
            (*i)->getSwitch()->removeConnectionTo(routerLabel);
            (*i)->getRouter()->removeConnectionToSwitch(switchLabel);
            return;
        }
    }
}

void BipartiteGraph::removeLinkRS(string routerLabel, string subnetLabel)
{
    for(list<LinkRouterSubnet*>::iterator i = linksRS.begin(); i != linksRS.end(); ++i)
    {
        string curRouterLabel = (*i)->getRouter()->getLabel();
        string curSubnetLabel = (*i)->getSubnet()->getLabel();
        
        if(routerLabel.compare(curRouterLabel) == 0 && subnetLabel.compare(curSubnetLabel) == 0)
        {
            linksRS.erase(i--);
            (*i)->getRouter()->removeConnectionToSubnet(subnetLabel);
            (*i)->getSubnet()->removeConnectionTo(routerLabel);
            return;
        }
    }
}

string BipartiteGraph::routersToString()
{
    stringstream routersStream;
    for(list<BipartiteRouter*>::iterator i = routers.begin(); i != routers.end(); ++i)
    {
        string label = (*i)->getLabel();
        routersStream << label << " - ";
        
        if((*i)->getType() == BipartiteRouter::T_IMAGINARY)
        {
            routersStream << "Imaginary" << endl;
        }
        else
        {
            routersStream << (*i)->getAssociatedRouter()->toString() << endl;
        }
    }
    
    return routersStream.str();
}

string BipartiteGraph::subnetsToString()
{
    stringstream subnetsStream;
    for(list<BipartiteSubnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        string label = (*i)->getLabel();
        subnetsStream << label << " - ";
        
        if((*i)->getType() == BipartiteSubnet::T_IMAGINARY)
        {
            subnetsStream << "Imaginary" << endl;
        }
        else
        {
            string subnetStr = (*i)->getAssociatedSubnet()->getInferredNetworkAddressString();
            subnetsStream << subnetStr << endl;
        }
    }
    
    return subnetsStream.str();
}

string BipartiteGraph::linksSRToString()
{
    stringstream linksSRStream;
    for(list<LinkSwitchRouter*>::iterator i = linksSR.begin(); i != linksSR.end(); ++i)
    {
        string labelSwitch = (*i)->getSwitch()->getLabel();
        string labelRouter = (*i)->getRouter()->getLabel();
        linksSRStream << labelSwitch << " - " << labelRouter << endl;
    }
    
    return linksSRStream.str();
}

string BipartiteGraph::linksRSToString()
{
    stringstream linksRSStream;
    for(list<LinkRouterSubnet*>::iterator i = linksRS.begin(); i != linksRS.end(); ++i)
    {
        string labelRouter = (*i)->getRouter()->getLabel();
        string labelSubnet = (*i)->getSubnet()->getLabel();
        linksRSStream << labelRouter << " - " << labelSubnet << endl;
    }
    
    return linksRSStream.str();
}
