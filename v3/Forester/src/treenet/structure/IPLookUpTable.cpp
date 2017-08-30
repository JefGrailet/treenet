/*
 * IPLookUpTable.cpp
 *
 *  Created on: Sep 29, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in IPLookUpTable.h (see this file to learn further about the goals 
 * of such class).
 */
 
#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include <iomanip>

using namespace std;

#include "IPLookUpTable.h"

IPLookUpTable::IPLookUpTable(unsigned short nbIPIDs)
{
    this->haystack = new list<IPTableEntry*>[SIZE_TABLE];
    this->nbIPIDs = nbIPIDs;
}

IPLookUpTable::~IPLookUpTable()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> IPList = this->haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            delete (*j);
        }
        haystack[i].clear();
    }
    delete[] haystack;
}

bool IPLookUpTable::isEmpty()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> IPList = this->haystack[i];
        if(IPList.size() > 0)
            return true;
    }
    return false;
}

IPTableEntry *IPLookUpTable::create(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<IPTableEntry*> *IPList = &(this->haystack[index]);
    
    for(list<IPTableEntry*>::iterator i = IPList->begin(); i != IPList->end(); ++i)
    {
        if((*i)->getULongAddress() == needle.getULongAddress())
        {
            return NULL;
        }
    }
    
    IPTableEntry *newEntry = new IPTableEntry(needle, this->nbIPIDs);
    IPList->push_back(newEntry);
    IPList->sort(IPTableEntry::compare);
    return newEntry;
}

IPTableEntry *IPLookUpTable::createAnyway(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<IPTableEntry*> *IPList = &(this->haystack[index]);
    IPTableEntry *newEntry = new IPTableEntry(needle, this->nbIPIDs);
    IPList->push_back(newEntry);
    return newEntry;
}

IPTableEntry *IPLookUpTable::lookUp(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<IPTableEntry*> *IPList = &(this->haystack[index]);
    
    for(list<IPTableEntry*>::iterator i = IPList->begin(); i != IPList->end(); ++i)
    {
        if((*i)->getULongAddress() == needle.getULongAddress())
        {
            return (*i);
        }
    }
    
    return NULL;
}

void IPLookUpTable::outputDictionnary(string filename)
{
    string output = "";
    
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> IPList = this->haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            string curStr = cur->toString();
            
            if(!curStr.empty())
                output += curStr + "\n";
        }
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void IPLookUpTable::outputFingerprints(string filename)
{
    string output = "";
    
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> IPList = this->haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            if(cur->isProcessedForAR())
            {
                string curStr = cur->toStringFingerprint();
                
                if(!curStr.empty())
                    output += curStr + "\n";
            }
        }
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void IPLookUpTable::clearAliasHints()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> IPList = this->haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            
            cur->resetARHints();
        }
    }
}
