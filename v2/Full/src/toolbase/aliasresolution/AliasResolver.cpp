/*
 * AliasResolver.cpp
 *
 *  Created on: Oct 20, 2015
 *      Author: grailet
 *
 * Implements the class defined in AliasResolver.h (see this file to learn further about the goals 
 * of such class).
 */

#include <list>
using std::list;
#include <cmath>

#include "AliasResolver.h"

AliasResolver::AliasResolver(TreeNETEnvironment *env)
{
    this->env = env;
}

AliasResolver::~AliasResolver()
{
}

bool AliasResolver::Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff)
{
    // The first condition is just in case; normally calling code checks it first
    if(ip1 != NULL && ip1->hasIPIDData() && ip2 != NULL && ip2->hasIPIDData())
    {
        unsigned short nbIPIDs = env->getNbIPIDs();
        
        // Finds a succession of tokens and their respective IDs for both IPs
        unsigned short x1 = 0, x2 = 0, y1 = 0, y2 = 0, z1 = 0, z2 = 0;
        bool success = false;
        for(unsigned short i = 0; i < nbIPIDs - 2; i++)
        {
            unsigned short token_x1, token_x2;
            token_x1 = ip1->getProbeToken(i);
            token_x2 = ip2->getProbeToken(i);
        
            x1 = ip1->getIPIdentifier(i);
            x2 = ip2->getIPIdentifier(i);
            
            unsigned short token_y1, token_y2;
            token_y1 = ip1->getProbeToken(i + 1);
            token_y2 = ip2->getProbeToken(i + 1);
            
            if(token_x1 < token_y2 && token_x2 < token_y1)
            {
                y1 = ip1->getIPIdentifier(i + 1);
                y2 = ip2->getIPIdentifier(i + 1);
            
                unsigned short token_z1, token_z2;
                token_z1 = ip1->getProbeToken(i + 2);
                token_z2 = ip2->getProbeToken(i + 2);
                
                if(token_y1 < token_z2 && token_y2 < token_z1)
                {
                    z1 = ip1->getIPIdentifier(i + 2);
                    z2 = ip2->getIPIdentifier(i + 2);
                    success = true;
                }
            }
        }
        
        // Ally is possible
        if(success)
        {
            unsigned int diffx1y2, diffx2y1, diffy1z2, diffy2z1;
            if(x1 < y2)
                diffx1y2 = y2 - x1;
            else
                diffx1y2 = (65535 - x1) + y2;
            
            if(x2 < y1)
                diffx2y1 = y1 - x2;
            else
                diffx2y1 = (65535 - x2) + y1;
                
            if(y1 < z2)
                diffy1z2 = z2 - y1;
            else
                diffy1z2 = (65535 - y1) + z2;
            
            if(y2 < z1)
                diffy2z1 = z1 - y2;
            else
                diffy2z1 = (65535 - y2) + z1;
            
            // x1 < y2 < z1 must hold as well as x2 < y1 < z2
            if(diffx1y2 <= maxDiff && diffx2y1 <= maxDiff 
            && diffy1z2 <= maxDiff && diffy2z1 <= maxDiff)
            {
                return true;
            }
            
            return false;
        }
    }
    return false;
}

void AliasResolver::computeVelocity(IPTableEntry *ip)
{
    // The first condition is just in case; normally calling code checks it first
    if(ip != NULL && ip->hasIPIDData())
    {
        // Alias resolution parameters
        unsigned short nbIPIDs = env->getNbIPIDs();
        unsigned short maxRollovers = env->getMaxRollovers();
        double maxError = env->getMaxError();
    
        // Delay, IP IDs of the first time interval
        double d0 = (double) ip->getDelay(0);
        double b0 = (double) ip->getIPIdentifier(0);
        double b1 = (double) ip->getIPIdentifier(1);
        
        double x = 0.0;
        double v[nbIPIDs - 1];
        
        bool success = false;
        for(unsigned short i = 0; i < maxRollovers; i++)
        {
            // Computing speed for x
            if(b1 > b0)
                v[0] = (b1 - b0 + 65535 * x) / d0;
            else
                v[0] = (b1 + (65535 - b0) + 65535 * x) / d0;
            
            success = true;
            for(unsigned short j = 1; j < nbIPIDs - 1; j++)
            {
                // Computing speed for cur
                double b_j = (double) ip->getIPIdentifier(j);
                double b_j_plus_1 = (double) ip->getIPIdentifier(j + 1);
                double d_j = (double) ip->getDelay(j);
                double cur = 0.0;
                
                cur += (d_j / d0) * x;
                
                if(b_j_plus_1 > b_j)
                    cur -= ((b_j_plus_1 - b_j) / 65535);
                else
                    cur -= ((b_j_plus_1 + (65535 - b_j)) / 65535);
                
                if(b1 > b0)
                    cur += (((d_j * b1) - (d_j * b0)) / (65535 * d0));
                else
                    cur += (((d_j * b1) + (d_j * (65535 - b0))) / (65535 * d0));
                
                // Flooring/ceiling cur
                double floorCur = floor(cur);
                double ceilCur = ceil(cur);
                
                double selectedCur = 0.0;
                double gap = 0.0;
                
                if((cur - floorCur) > (ceilCur - cur))
                {
                    selectedCur = ceilCur;
                    gap = ceilCur - cur;
                }
                else
                {
                    selectedCur = floorCur;
                    gap = cur - floorCur;
                }
                
                // Storing speed of current time interval
                if(selectedCur > 0.0 && gap <= maxError)
                {
                    if(b_j_plus_1 > b_j)
                        v[j] = (b_j_plus_1 - b_j + 65535 * selectedCur) / d_j;
                    else
                        v[j] = (b_j_plus_1 + (65535 - b_j) + 65535 * selectedCur) / d_j;
                }
                else
                {
                    success = false;
                    break;
                }
            }
            
            if(success)
            {
                break;
            }
            
            x += 1.0;
        }
        
        if(success)
        {
            double maxV = v[0], minV = v[0];
            for(unsigned short i = 0; i < nbIPIDs - 1; i++)
            {
                if(v[i] > maxV)
                    maxV = v[i];
                if(v[i] < minV)
                    minV = v[i];
            }
            
            ip->setVelocityLowerBound(minV);
            ip->setVelocityUpperBound(maxV);
        }
        else
        {
            // "Infinite" velocity: [0.0, 65535.0]
            ip->setVelocityLowerBound(0.0);
            ip->setVelocityUpperBound(65535.0);
        }
    }
}

bool AliasResolver::velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1 == NULL || ip2 == NULL)
        return false;

    double low1 = ip1->getVelocityLowerBound();
    double low2 = ip2->getVelocityLowerBound();
    double up1 = ip1->getVelocityUpperBound();
    double up2 = ip2->getVelocityUpperBound();

    if(up1 == 0.0 || up2 == 0.0)
        return false;

    // Particular case of "infinite" velocity
    if(up1 == 65535.0 || up2 == 65535.0)
    {
        if(up1 == up2)
            return true;
        return false;
    }
    
    // Computes tolerance value
    double tolerance = env->getBaseTolerance();
    if((low1 / (tolerance * 10)) > 1.0 && (low2 / (tolerance * 10)) > 1.0)
        tolerance *= (low1 / (tolerance * 10));
    
    // Finds largest intervall and extends it with tolerance
    double low1Bis = low1, low2Bis = low2, up1Bis = up1, up2Bis = up2;
    if((up1 - low1) > (up2 - low2))
    {
        low1Bis = low1 - tolerance;
        up1Bis = up1 + tolerance;
    }
    else
    {
        low2Bis = low2 - tolerance;
        up2Bis = up2 + tolerance;
    } 
    
    // Test overlap with tolerance
    if(low1Bis <= up2Bis && up1Bis >= low2Bis)
        return true;
    
    return false;
}

bool AliasResolver::reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2)
{
    // Just in case
    if(ip1 == NULL || ip2 == NULL)
        return false;
    
    string hostName1 = ip1->getStoredHostName();
    string hostName2 = ip2->getStoredHostName();
    
    if(!hostName1.empty() && !hostName2.empty())
    {
        list<string> hn1Chunks;
        list<string> hn2Chunks;
        string element;
        
        stringstream hn1(hostName1);
        while (std::getline(hn1, element, '.'))
        {
            hn1Chunks.push_front(element);
        }
        
        stringstream hn2(hostName2);
        while (std::getline(hn2, element, '.'))
        {
            hn2Chunks.push_front(element);
        }
        
        if(hn1Chunks.size() == hn2Chunks.size())
        {
            unsigned short size = (unsigned short) hn1Chunks.size();
            unsigned short similarities = 0;
            
            list<string>::iterator itBis = hn1Chunks.begin();
            for(list<string>::iterator it = hn2Chunks.begin(); it != hn2Chunks.end(); ++it)
            {
                if((*it).compare((*itBis)) == 0)
                {
                    similarities++;
                    itBis++;
                }
                else
                    break;
            }
            
            if(similarities >= (size - 1))
                return true;
            return false;
        }
    }
    return false;
}

list<Router> AliasResolver::resolve(NetworkTreeNode *internal)
{
    list<InetAddress> interfaces = internal->listInterfaces();
    IPLookUpTable *table = env->getIPTable();
    
    // Remove duplicata (possible because ingress interface of neighborhood can be a contra-pivot)
    InetAddress previous(0);
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        InetAddress current = (*i);
        if(current == previous)
            interfaces.erase(i--);
        
        previous = current;
    }
    
    // Starts association...
    list<Router> result;
    while(interfaces.size() > 0)
    {
        InetAddress cur = interfaces.front();
        interfaces.pop_front();
        IPTableEntry *entryCur = table->lookUp(cur);
        if(entryCur == NULL)
            continue;
        
        this->computeVelocity(entryCur);
        
        // If not eligible for IP ID inference or reverse DNS, creates a router for this IP only
        bool okForIPID = entryCur->hasIPIDData();
        if(!okForIPID && entryCur->getStoredHostName().empty())
        {
            Router curRouter;
            curRouter.addInterface(InetAddress(cur), RouterInterface::FIRST_IP);
            result.push_back(curRouter);
            continue;
        }
        
        // Creates router
        Router curRouter;
        curRouter.addInterface(InetAddress(cur), RouterInterface::FIRST_IP);
        
        // Starts going through interfaces list
        for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        {
            IPTableEntry *entryI = table->lookUp((*i));
            if(entryI == NULL)
                continue;
            bool iOkForIPID = entryI->hasIPIDData();
            
            // Tries Ally, IP ID velocity comparison (i.e. like RadarGun)
            if(okForIPID && iOkForIPID)
            {
                this->computeVelocity(entryI);
                
                // We must compare to every IP stored in the router at this point
                list<RouterInterface> *rList = curRouter.getInterfacesList();
                for(list<RouterInterface>::iterator k = rList->begin(); k != rList->end(); ++k)
                {
                    RouterInterface curInterface = (*k);
                    IPTableEntry *entryK = table->lookUp(curInterface.ip);
                    if(entryK == NULL)
                        continue;
                    
                    if(this->Ally(entryI, entryK, MAX_IP_ID_DIFFERENCE))
                    {
                        curRouter.addInterface(InetAddress((*i)), RouterInterface::ALLY);
                        interfaces.erase(i--);
                        break;
                    }
                    else if(this->velocityOverlap(entryI, entryK))
                    {
                        curRouter.addInterface(InetAddress((*i)), RouterInterface::IPID_VELOCITY);
                        interfaces.erase(i--);
                        break;
                    }
                }
            }
            // Tries reverse DNS if IP ID-based alias resolution is not possible
            else if(this->reverseDNS(entryCur, entryI))
            {
                curRouter.addInterface(InetAddress((*i)), RouterInterface::REVERSE_DNS);
                interfaces.erase(i--);
            }
        }
        result.push_back(curRouter);
    }
    
    /*
     * Some post-processing: removes the routers consisting of a single interface which happens 
     * to be a candidate contra-pivot of an ODD subnet, except if it is among the labels of this 
     * node.
     *
     * N.B.: checking the interface appears in the subnet responsive IPs list is enough, as the 
     * pivots were not listed at all in the potential interfaces.
     */
     
    for(list<Router>::iterator i = result.begin(); i != result.end(); ++i)
    {
        if((*i).getNbInterfaces() == 1)
        {
            InetAddress singleInterface = (*i).getInterfacesList()->front().ip;
            list<NetworkTreeNode*> *children = internal->getChildren();
            for(list<NetworkTreeNode*>::iterator j = children->begin(); j != children->end(); ++j)
            {
                if((*j)->isLeaf())
                {
                    SubnetSite *ss = (*j)->getAssociatedSubnet();
                    
                    if(ss->getRefinementStatus() == SubnetSite::ODD_SUBNET && 
                       ss->hasLiveInterface(singleInterface))
                    {
                        bool isALabel = false;
                        list<InetAddress> *labels = internal->getLabels();
                        for(list<InetAddress>::iterator k = labels->begin(); k != labels->end(); ++k)
                        {
                            if((*k) == singleInterface)
                            {
                                isALabel = true;
                                break;
                            }
                        }
                        
                        if(!isALabel)
                        {
                            result.erase(i--);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}
