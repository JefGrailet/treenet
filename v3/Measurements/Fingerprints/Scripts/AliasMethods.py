#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes the ratio of application of each alias resolution method via stack bar charts, just
# like AliasProportions.py. The required command line is also similar; however, here, the 
# bipartite file is required.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python AliasMethods.py [year] [date] [path to AS file]")
        sys.exit()
    
    yearOfMeasurements = str(sys.argv[1])
    dateOfMeasurements = str(sys.argv[2])
    ASFilePath = str(sys.argv[3])
    
    # Parses AS file
    if not os.path.isfile(ASFilePath):
        print("AS file does not exist")
        sys.exit()

    with open(ASFilePath) as f:
        ASesRaw = f.read().splitlines()
        
    # For this particular file, we do not class by type. We remove the :[type] part.
    ASes = []
    for i in range(0, len(ASesRaw)):
        splitted = ASesRaw[i].split(':')
        ASes.append(splitted[0])

    # Computes the required data
    correctlyParsedASes = []
    ratioUDP = []
    ratioAlly = []
    ratioVelocity = []
    ratioGroup = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns" # TODO: edit this
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePath += dateOfMeasurements + "/Bipartite " + ASes[i]
        dataFilePath += "_" + dateOfMeasurements
        
        # Checks existence of the file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist")
            sys.exit()
        else:
            correctlyParsedASes.append(ASes[i])
        
        # Parses it, count occurrences of healthy, echo, random and "other" counters
        with open(dataFilePath) as f:
            fileLines = f.read().splitlines()
        
        j = 0
        nbGoodAliasLists = 0
        nbUDP = 0
        nbAlly = 0
        nbVelocity = 0
        nbGroup = 0
        while fileLines[j]:
            splitted = fileLines[j].split(' - ')
            aliasList = splitted[1]
            
            nbIPs = aliasList.count(',')
            if nbIPs < 1:
                j += 1
                continue
            
            nbGoodAliasLists += 1
            
            occurrences = np.zeros((5, 1))
            occurrences[0] = aliasList.count('UDP')
            occurrences[1] = aliasList.count('Ally')
            occurrences[2] = aliasList.count('Velocity')
            occurrences[3] = aliasList.count('Echo group')
            occurrences[4] = aliasList.count('Random group')
            
            maximum = 0
            indexMax = 0
            for k in range(0, 5):
                if occurrences[k] > maximum:
                    maximum = occurrences[k]
                    indexMax = k
            
            if indexMax == 0:
                nbUDP += 1
            elif indexMax == 1:
                nbAlly += 1
            elif indexMax == 2:
                nbVelocity += 1
            else:
                nbGroup += 1
        
            j += 1
        
        # Computes ratios
        ratioUDP.append((float(nbUDP) / float(nbGoodAliasLists)) * 100)
        ratioAlly.append((float(nbAlly) / float(nbGoodAliasLists)) * 100)
        ratioVelocity.append((float(nbVelocity) / float(nbGoodAliasLists)) * 100)
        ratioGroup.append((float(nbGroup) / float(nbGoodAliasLists)) * 100)

    ind = np.arange(len(correctlyParsedASes)) # The x locations
    width = 0.8
    center = 0.5
    padding = 0.1
    
    # Font for labels and ticks
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':21}
    
    hfont2 = {'fontname':'serif',
             'fontsize':21}

    plt.figure(figsize=(11,7))
    
    bottom1 = ratioUDP
    bottom2 = np.zeros((len(correctlyParsedASes), 1))
    for i in range(0, len(correctlyParsedASes)):
        bottom2[i] = bottom1[i] + ratioAlly[i]
    bottom3 = np.zeros((len(correctlyParsedASes), 1))
    for i in range(0, len(correctlyParsedASes)):
        bottom3[i] = bottom2[i] + ratioVelocity[i]

    p1 = plt.bar(ind + padding, ratioUDP, width, color='#000000')
    p2 = plt.bar(ind + padding, ratioAlly, width, color='#888888', bottom=bottom1)
    p3 = plt.bar(ind + padding, ratioVelocity, width, color='#D0D0D0', bottom=bottom2)
    p4 = plt.bar(ind + padding, ratioGroup, width, color='#F0F0F0', bottom=bottom3)
    
    plt.ylabel('Proportion of aliases (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,20])
    plt.xticks(ind + center, range(1,21,1), **hfont2)
    plt.yticks(np.arange(0, 101, 10), **hfont2)
    
    plt.rc('font', family='serif', size=14)
    plt.legend((p1[0], p2[0], p3[0], p4[0]), 
               ('Address-based', 'Ally', 'IP-ID Velocity', 'Group'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("AliasMethods_" + yearOfMeasurements + "_" + dateOfMeasurements + "_" + ASFilePath + ".pdf")
    plt.clf()
