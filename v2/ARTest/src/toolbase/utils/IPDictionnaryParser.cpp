/*
 * IPDictionnaryParser.cpp
 *
 *  Created on: Jan 25, 2016
 *      Author: grailet
 *
 * Implements the class defined in IPDictionnaryParser.h.
 */

#include <cstdlib>
#include <algorithm>
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;

#include "IPDictionnaryParser.h"

IPDictionnaryParser::IPDictionnaryParser(TreeNETEnvironment *env)
{
    this->env = env;
}

IPDictionnaryParser::~IPDictionnaryParser()
{
}

list<string> IPDictionnaryParser::explode(string input, char delimiter)
{
    list<string> result;
    stringstream ss(input);
    string chunk;
    
    while(std::getline(ss, chunk, delimiter))
        result.push_back(chunk);
    
    return result;
}

void IPDictionnaryParser::parseInputFile(string inputFileContent)
{
    ostream *out = env->getOutputStream();
    IPLookUpTable *dictionnary = env->getIPTable();
    
    stringstream ss(inputFileContent);
    string targetStr;
    
    unsigned int nbLine = 0;
    while (std::getline(ss, targetStr, '\n'))
    {
        nbLine++;
    
        // Empty lines are not parsed (should not occur, actually)
        if(targetStr.size() == 0)
            continue;
        
        // Values to parse
        InetAddress liveIP(0);
        list<unsigned int> tokensLs;
        list<unsigned int> IPIDsLs;
        list<unsigned int> delaysLs;
        string hostName = "";
        
        // First separator: the ":" character
        size_t pos = targetStr.find(':');
        if(pos == std::string::npos)
        {
            // No ":" means we should only have an IP
            try
            {
                liveIP.setInetAddress(targetStr);
            }
            catch (InetAddressException &e)
            {
                (*out) << "Malformed/Unrecognized IP \"" + targetStr;
                (*out) << "\" at line " << nbLine << "." << endl;
                continue;
            }
            
            // Saving in IP dictionnary
            dictionnary->createAnyway(liveIP);

            continue;
        }
        
        string IPStr = targetStr.substr(0, pos);
        string ARHintsStr = targetStr.substr(pos + 1);

        try
        {
            liveIP.setInetAddress(IPStr);
        }
        catch (InetAddressException &e)
        {
            (*out) << "Malformed/Unrecognized IP \"" + IPStr;
            (*out) << "\" at line " << nbLine << "." << endl;
            continue;
        }
        
        // Saving in IP dictionnary (newEntry will also be used afterwards for A.R. hints)
        IPTableEntry *newEntry = NULL;
        newEntry = dictionnary->createAnyway(liveIP);
        
        // Skips if entry previously existed
        if(newEntry == NULL)
            continue;
        
        // Removes suffix of ARHintsStr (i.e. " - [lowerVelocity,upperVelocity]")
        size_t pos3 = ARHintsStr.find('-');
        if(pos3 != std::string::npos)
            ARHintsStr = ARHintsStr.substr(0, pos3 - 1);
        
        // Parsing alias resolution hints starts here
        list<string> hints = explode(ARHintsStr, ',');
        size_t nbHints = hints.size();
        
        /*
         * Host name first, because it is always last part, and is easy to disambiguate from 
         * token/IP-ID pairs due to the absence of the character ";".
         */
        
        string candidateHostName = hints.back();
        if(candidateHostName.find(';') == std::string::npos)
        {
            hostName = candidateHostName;
            
            // Last chunk is removed to only keep delays and token/IP-ID pairs
            hints.pop_back();
            nbHints--;
        }
        
        if(hints.size() == 0)
        {
            // No token/IP-ID pair, just DNS: goes to next iteration without any print out
            continue;
        }
        
        /*
         * Policy regarding the amount of IP-IDS: during alias resolution, TreeNET Reader will use 
         * its input parameters, even if the parsed IP dictionnary contained more. For example, if 
         * -a has the value "4" but the dictionnary lists up to 5 IP-IDs, only the 4 first will be 
         * considered. Reciprocally, if -a has "5" but the dictionnary lists at most 4 IP-IDs, 
         * these IP-IDs will not be parsed.
         */
        
        unsigned short expectedSize = (env->getNbIPIDs() * 2) - 1;
        unsigned short amountIPIDs = (hints.size() + 1) / 2;
        if(amountIPIDs < env->getNbIPIDs())
        {
            (*out) << "Too few IP-IDs at line " << nbLine << ": expected " << env->getNbIPIDs();
            (*out) << ", only got " << amountIPIDs << "." << endl;
            continue;
        }
        
        bool fail = false;
        for(unsigned short i = 0; i < expectedSize; i++)
        {
            string curHint = hints.front();
            hints.pop_front();
        
            // Token/IP-ID pair
            if((i % 2) == 0)
            {
                if(curHint.find(';') == std::string::npos)
                {
                    fail = true;
                    break;
                }
                else
                {
                    list<string> splitted = explode(curHint, ';');
                    if(splitted.size() > 2)
                    {
                        fail = true;
                        break;
                    }
                    
                    unsigned int token = std::atoi(splitted.front().c_str());
                    tokensLs.push_back(token);
                    
                    splitted.pop_front();
                    
                    unsigned int IPID = std::atoi(splitted.front().c_str());
                    IPIDsLs.push_back(IPID);
                }
            }
            // Delay
            else
            {
                if(curHint.find(';') == std::string::npos)
                {
                    unsigned int delay = std::atoi(curHint.c_str());
                    delaysLs.push_back(delay);
                }
                else
                {
                    fail = true;
                    break;
                }
            }
        }
        
        if(fail)
        {
            (*out) << "Badly formatted IP-IDs at line " << nbLine << ". They will not be ";
            (*out) << "parsed." << endl;
            continue;
        }
        
        // Now saving the parsed IP-IDs and delays.
        unsigned short expectedIPIDs = env->getNbIPIDs();
        for(unsigned short i = 0; i < expectedIPIDs; i++)
        {
            newEntry->setProbeToken(i, tokensLs.front());
            newEntry->setIPIdentifier(i, IPIDsLs.front());
            
            tokensLs.pop_front();
            IPIDsLs.pop_front();
            
            if(i < expectedIPIDs - 1)
            {
                newEntry->setDelay(i, delaysLs.front());
                
                delaysLs.pop_front();
            }
        }
        
        // We're done here.
    }
}
