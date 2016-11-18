/*
 * Soil.cpp
 *
 *  Created on: Sept 8, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Soil.h (see this file to learn further about the goals of such 
 * class).
 */

#include <string>
using std::string;

// 3 next lines are for outputting data in text files
#include <fstream>
#include <sys/stat.h> // For CHMOD edition
using std::ofstream;

#include "Soil.h"

Soil::Soil()
{
    this->subnetMap = new list<SubnetMapEntry*>[SIZE_SUBNET_MAP];
}

Soil::~Soil()
{
    // Deletes subnet map
    for(unsigned int i = 0; i < SIZE_SUBNET_MAP; i++)
    {
        list<SubnetMapEntry*> sList = this->subnetMap[i];
        for(list<SubnetMapEntry*>::iterator i = sList.begin(); i != sList.end(); ++i)
        {
            delete (*i);
        }
    }
    delete[] subnetMap;
    
    // Deletes tree(s)
    for(list<NetworkTree*>::iterator i = roots.begin(); i != roots.end(); i++)
    {
        delete (*i);
    }
}

void Soil::insertMapEntries(list<SubnetMapEntry*> newEntries)
{
    for(list<SubnetMapEntry*>::iterator i = newEntries.begin(); i != newEntries.end(); ++i)
    {
        InetAddress needle = (*i)->subnet->getPivot();
        unsigned long index = (needle.getULongAddress() >> 12);
        this->subnetMap[index].push_back((*i));
    }
}

void Soil::sortMapEntries()
{
    for(unsigned int i = 0; i < SIZE_SUBNET_MAP; i++)
    {
        if(this->subnetMap[i].size() > 1)
            this->subnetMap[i].sort(SubnetMapEntry::compare);
    }
}

SubnetMapEntry *Soil::getSubnetContaining(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<SubnetMapEntry*> subnetList = this->subnetMap[index];
    
    for(list<SubnetMapEntry*>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
    {
        SubnetMapEntry *entry = (*i);
        if(entry->subnet->contains(needle))
            return entry;
    }
    return NULL;
}

void Soil::outputSubnets(string filename)
{
    string finalOutput = "";
    for(list<NetworkTree*>::iterator i = roots.begin(); i != roots.end(); i++)
    {
        finalOutput += (*i)->writeSubnets();
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << finalOutput;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
