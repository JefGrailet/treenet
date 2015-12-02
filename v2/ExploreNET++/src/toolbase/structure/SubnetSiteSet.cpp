/*
 * SubnetSiteSet.cpp
 *
 *  Created on: Oct 9, 2014
 *      Author: grailet
 *
 * Implements the class defined in SubnetSiteSet.h (see this file to learn further about the 
 * goals of such class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include "SubnetSiteSet.h"
#include "../../common/inet/NetworkAddress.h"

using namespace std;

SubnetSiteSet::SubnetSiteSet()
{
}

SubnetSiteSet::~SubnetSiteSet()
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        delete (*i);
    }
    siteList.clear();
}

void SubnetSiteSet::addSite(SubnetSite *ss)
{
    siteList.push_back(ss);
    siteList.sort(SubnetSite::compare);
}

void SubnetSiteSet::outputAsFile(string filename)
{
    string output = "";
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        string cur = ss->refinedToString();
        
        if(!cur.empty())
            output += cur + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
