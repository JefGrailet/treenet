/*
 * IPDictionnaryParser.cpp
 *
 *  Created on: Jan 6, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in IPDictionnaryParser.h.
 */

#include <cstdlib>
#include <algorithm>
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;
#include <iomanip>
using std::setprecision;
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::endl;
using std::flush;

#include "IPDictionnaryParser.h"

IPDictionnaryParser::IPDictionnaryParser(TreeNETEnvironment *env)
{
    this->env = env;
    this->parsedLines = 0;
    this->badLines = 0;
    this->unusableLines = 0;
    this->totalLines = 0;
}

IPDictionnaryParser::~IPDictionnaryParser()
{
}

list<string> IPDictionnaryParser::explode(string input, char delimiter)
{
    list<string> result;

    // Checks if delimiter is there; if not return a single-element list
    size_t pos = input.find(delimiter);
    if(pos == std::string::npos)
    {
        result.push_back(input);
        return result;
    }
    
    // Splits the input string according to delimiter
    stringstream ss(input);
    string chunk;
    while(std::getline(ss, chunk, delimiter))
        result.push_back(chunk);
    
    return result;
}

bool IPDictionnaryParser::parse(string inputFileName)
{
    unsigned short displayMode = env->getDisplayMode();
    IPLookUpTable *dictionnary = env->getIPTable();
    
    ostream *out = env->getOutputStream();
    (*out) << "Parsing " << inputFileName << "..." << endl;
    
    // Resetting count fields for next parsing.
    this->parsedLines = 0;
    this->badLines = 0;
    this->unusableLines = 0;
    this->totalLines = 0;
    
    string inputFileContent = "";
    ifstream inFile;
    inFile.open((inputFileName).c_str());
    if(inFile.is_open())
    {
        inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                (std::istreambuf_iterator<char>()));
        
        inFile.close();
    }
    else
    {
        (*out) << "File " << inputFileName << " does not exist.\n";
        (*out) << "IP dictionnary will be filled with interfaces found in subnets or in their ";
        (*out) << "respective routes.\n" << endl;
        return false;
    }
    
    stringstream ss(inputFileContent);
    string targetStr;
    
    unsigned int nbLine = 0;
    while (std::getline(ss, targetStr, '\n'))
    {
        nbLine++;
    
        // Empty lines are not parsed (should not occur, actually)
        if(targetStr.size() == 0)
            continue;
        
        this->totalLines++;
        
        // Values to parse
        InetAddress liveIP(0);
        unsigned char TTL;
        list<unsigned int> tokensLs;
        list<unsigned int> IPIDsLs;
        list<unsigned int> delaysLs;
        string hostName = "";
        
        // First separator: the ":" character
        size_t pos = targetStr.find(':');
        if(pos == std::string::npos)
        {
            // No ":" means we should only have IP and TTL available
            size_t pos2 = targetStr.find('-');
            if(pos2 == std::string::npos)
            {
                this->unusableLines++;
                if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    (*out) << "Line " << nbLine << " does not match any known syntax." << endl;
                continue;
            }
            
            string IPStr = targetStr.substr(0, pos2 - 1);
            string TTLStr = targetStr.substr(pos2 + 1);
            
            try
            {
                liveIP.setInetAddress(IPStr);
            }
            catch (InetAddressException &e)
            {
                this->unusableLines++;
                if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                {
                    (*out) << "Malformed/Unrecognized IP \"" + IPStr;
                    (*out) << "\" at line " << nbLine << "." << endl;
                }
                continue;
            }
            TTL = (unsigned char) std::atoi(TTLStr.c_str());
            
            // Saving in IP dictionnary
            IPTableEntry *newEntry = NULL;
            if(!env->usingMergingAtParsing())
                newEntry = dictionnary->createAnyway(liveIP);
            else
                newEntry = dictionnary->create(liveIP);
            
            // Skips if entry previously existed
            if(newEntry == NULL)
                continue;
            
            newEntry->setTTL(TTL);
            
            this->parsedLines++;
            continue;
        }
        
        string IPAndTTLStr = targetStr.substr(0, pos);
        string ARHintsStr = targetStr.substr(pos + 2);
        
        size_t pos2 = IPAndTTLStr.find('-');
        if(pos2 == std::string::npos)
        {
            this->badLines++;
            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                (*out) << "Line " << nbLine << " does not match expected IP - TTL syntax." << endl;
            continue;
        }
        
        string IPStr = IPAndTTLStr.substr(0, pos2 - 1);
        string TTLStr = IPAndTTLStr.substr(pos2 + 1);
        
        try
        {
            liveIP.setInetAddress(IPStr);
        }
        catch (InetAddressException &e)
        {
            this->badLines++;
            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
            {
                (*out) << "Malformed/Unrecognized IP \"" + IPStr;
                (*out) << "\" at line " << nbLine << "." << endl;
            }
            continue;
        }
        TTL = (unsigned char) std::atoi(TTLStr.c_str());
        
        // Saving in IP dictionnary (newEntry will also be used afterwards for A.R. hints)
        IPTableEntry *newEntry = NULL;
        if(!env->usingMergingAtParsing())
            newEntry = dictionnary->createAnyway(liveIP);
        else
            newEntry = dictionnary->create(liveIP);
        
        // Skips if entry previously existed
        if(newEntry == NULL)
        {
            this->totalLines--;
            continue;
        }
        
        newEntry->setTTL(TTL);

        // Parsing alias resolution hints starts here; first checks that there is an initial TTL
        size_t pos3 = ARHintsStr.find(" - ");
        if(pos3 != std::string::npos)
        {
            string iTTLStr = ARHintsStr.substr(0, pos3);
            ARHintsStr = ARHintsStr.substr(pos3 + 3);
            
            unsigned char iTTL = (unsigned char) std::atoi(iTTLStr.c_str());
            newEntry->setEchoInitialTTL(iTTL);
        }
        
        /* 
         * We now look for and remove the " || [IP]" suffix. This corresponds to a pre-alias, a 
         * feature of TreeNET v3.3 which consists in running alias resolution only on labels of 
         * multi-label nodes to identify aliases between them, in order to get the best possible 
         * aggregates for the full alias resolution.
         */
        
        size_t pos4 = ARHintsStr.find(" || ");
        if(pos4 != std::string::npos)
        {
            string preAliasesStr = ARHintsStr.substr(pos4 + 4);
            ARHintsStr = ARHintsStr.substr(0, pos4);
            
            // Parses the pre-aliases
            list<string> preAliases = explode(preAliasesStr, ',');
            for(list<string>::iterator it = preAliases.begin(); it != preAliases.end(); ++it)
            {
                InetAddress preAlias(0);
                try
                {
                    preAlias.setInetAddress((*it));
                    newEntry->recordPreAlias(preAlias);
                }
                catch (InetAddressException &e)
                {
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    {
                        (*out) << "Malformed/Unrecognized pre-alias " << (*it);
                        (*out) << "\" at line " << nbLine << "." << endl;
                    }
                    continue;
                }
            }
        }
        
        /*
         * Next, we remove the " | [Yes],..." part that can occur at the end of the line, which 
         * corresponds to compliance with ICMP timestamp request and UDP. We also process this 
         * part after parsing it out. After, ARHintsStr should only contain the hints relative to 
         * IP ID-based techniques.
         */
        
        size_t pos5 = ARHintsStr.find(" | ");
        if(pos5 != std::string::npos)
        {
            string complianceStr = ARHintsStr.substr(pos5 + 3);
            ARHintsStr = ARHintsStr.substr(0, pos5);
            
            // There might be a comma, hinting there is compliance to both UDP and ICMP timestamp
            size_t pos6 = complianceStr.find(",");
            if(pos6 != std::string::npos)
            {
                newEntry->setReplyingToTSRequest(); // Anyway
                string replyingSrcIPStr = complianceStr.substr(pos6 + 1);
                InetAddress replyingSrcIP(0);
                
                try
                {
                    replyingSrcIP.setInetAddress(replyingSrcIPStr);
                    newEntry->setPortUnreachableSrcIP(replyingSrcIP);
                }
                catch (InetAddressException &e)
                {
                    this->badLines++;
                    if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                    {
                        (*out) << "Malformed/Unrecognized IP (source IP of ICMP Port ";
                        (*out) << "Unreachable) \"" + replyingSrcIPStr;
                        (*out) << "\" at line " << nbLine << "." << endl;
                    }
                    continue;
                }
            }
            // If no comma: we have to check if this is a "Yes" or an IP
            else
            {
                string compliancePrefix = complianceStr.substr(0, 3);
                if(compliancePrefix.compare("Yes") == 0)
                {
                    newEntry->setReplyingToTSRequest();
                }
                else
                {
                    InetAddress replyingSrcIP(0);
                
                    try
                    {
                        replyingSrcIP.setInetAddress(complianceStr);
                        newEntry->setPortUnreachableSrcIP(replyingSrcIP);
                    }
                    catch (InetAddressException &e)
                    {
                        this->badLines++;
                        if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
                        {
                            (*out) << "Malformed/Unrecognized IP (source IP of ICMP Port ";
                            (*out) << "Unreachable) \"" + complianceStr;
                            (*out) << "\" at line " << nbLine << "." << endl;
                        }
                        continue;
                    }
                }
            }
        }
       
        if(ARHintsStr.find(',') == std::string::npos)
        {
            // Echo counter
            if(ARHintsStr.compare("ECHO") == 0)
            {
                if(newEntry->getEchoInitialTTL() == 0)
                    newEntry->setEchoInitialTTL(255);
            
                newEntry->setCounterType(IPTableEntry::ECHO_COUNTER);
                newEntry->raiseFlagProcessed();
            }
            // Host name
            else
            {
                newEntry->setHostName(ARHintsStr);
            }
            this->parsedLines++;
            continue;
        }
        
        // Starts parsing IP IDs and host name
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
            newEntry->setHostName(hostName);
            
            // Last chunk is removed to only keep delays and token/IP ID pairs
            hints.pop_back();
            nbHints--;
        }
        
        if(hints.size() == 0)
        {
            // No token/IP ID pair, just DNS: goes to next iteration without any print out
            this->parsedLines++;
            continue;
        }
        
        // If a single "hint" that is equal to string "ECHO", already sets an IP ID counter type
        if(hints.size() == 1 && hints.front().compare("ECHO") == 0)
        {
            if(newEntry->getEchoInitialTTL() == 0)
                newEntry->setEchoInitialTTL(255);
        
            newEntry->setCounterType(IPTableEntry::ECHO_COUNTER);
            newEntry->raiseFlagProcessed();
            this->parsedLines++;
            continue;
        }
        
        /*
         * Policy regarding the amount of IP IDS: during alias resolution, TreeNET Reader will use 
         * its input parameters, even if the parsed IP dictionnary contained more. For example, if 
         * -a has the value "4" but the dictionnary lists up to 5 IP IDs, only the 4 first will be 
         * considered. Reciprocally, if -a has "5" but the dictionnary lists at most 4 IP IDs, 
         * these IP IDs will not be parsed.
         */
        
        unsigned short expectedSize = (env->getNbIPIDs() * 2) - 1;
        unsigned short amountIPIDs = (hints.size() + 1) / 2;
        if(amountIPIDs < env->getNbIPIDs())
        {
            this->badLines++;
            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
            {
                (*out) << "Too few IP-IDs at line " << nbLine << ": expected ";
                (*out) << env->getNbIPIDs() << ", only got " << amountIPIDs << "." << endl;
            }
            continue;
        }
        
        bool fail = false;
        for(unsigned short i = 0; i < expectedSize; i++)
        {
            string curHint = hints.front();
            hints.pop_front();
        
            // Token/IP ID pair
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
            this->badLines++;
            if(displayMode >= TreeNETEnvironment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
            {
                (*out) << "Badly formatted IP-IDs at line " << nbLine << ". They will not be ";
                (*out) << "parsed." << endl;
            }
            continue;
        }
        
        // Now saving the parsed IP IDs and delays.
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
        
        this->parsedLines++;
    }
    
    // Summary of parsing
    if(this->totalLines > 0)
        (*out) << "Total of parsed lines: " << this->totalLines << endl;
    if(this->parsedLines > 0)
    {
        float ratio = ((float) this->parsedLines / (float) this->totalLines) * 100;
        (*out) << "Fully parsed lines (correct format): " << this->parsedLines << " (";
        (*out) << setprecision(3) << ratio << "%)" << endl;
    }
    if(this->badLines > 0)
    {
        float ratio = ((float) this->badLines / (float) this->totalLines) * 100;
        (*out) << "Partially formatted lines (formatting errors): " << this->badLines << " (";
        (*out) << setprecision(3) << ratio << "%)" << endl;
    }
    if(this->unusableLines > 0)
    {
        float ratio = ((float) this->unusableLines / (float) this->totalLines) * 100;
        (*out) << "Unusable lines (bad format): " << this->unusableLines << " (";
        (*out) << setprecision(3) << ratio << "%)" << endl;
    }
    
    (*out) << "Parsing of " << inputFileName << " completed.\n" << flush;
    if(this->parsedLines == 0)
        (*out) << "No correct line was parsed. This might worsen subsequent tasks.\n";
    (*out) << endl;
    
    return true;
}
