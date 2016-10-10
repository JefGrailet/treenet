#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes the ratio of fingerprinted IPs which were actually aliased to other IPs and presents 
# the results in the same fashion as "FingerprintsAnalysis[...].py" scripts. The required command 
# line is also similar; however here only the .alias file is checked.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python AliasProportions.py [year] [date] [path to AS file]")
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
    ratioAliased = []
    ratioNonAliased = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns" #TODO: edit this
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePath += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        dataFilePath += ".alias"
        
        # Checks existence of the file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist")
            sys.exit()
        else:
            correctlyParsedASes.append(ASes[i])
        
        # Parses it and analyzes aliases
        with open(dataFilePath) as f:
            aliases = f.read().splitlines()
        
        nbAliases = len(aliases)
        integerData = np.zeros((2, 1)) # 0 = unaliased, 1 = aliased
        totalIPs = 0
        for j in range(0, nbAliases):
            splitted = aliases[j].split(' ')
            nbAliasedIPs = len(splitted)
            totalIPs += nbAliasedIPs
            if nbAliasedIPs == 1:
                integerData[1] += 1
            else:
                integerData[0] += nbAliasedIPs
        
        # Computes ratios
        ratioAliased.append((float(integerData[0]) / float(totalIPs)) * 100)
        ratioNonAliased.append((float(integerData[1]) / float(totalIPs)) * 100)

    ind = np.arange(len(correctlyParsedASes)) # The x locations
    width = 0.8
    center = 0.5
    padding = 0.1
    
    # Font for labels and ticks
    hfont = {'fontname':'serif',
             'fontsize':21}
    
    hfont2 = {'fontname':'serif',
             'fontsize':12}

    plt.figure(figsize=(11,7))

    p1 = plt.bar(ind + padding, ratioAliased, width, color='#F0F0F0')
    p2 = plt.bar(ind + padding, ratioNonAliased, width, color='#888888', bottom=ratioAliased)
    
    plt.ylabel('Proportion of fingerprinted IPs (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,20])
    #plt.xticks(ind + center, correctlyParsedASes, rotation=25, **hfont2)
    plt.xticks(ind + center, range(1,21,1), **hfont)
    plt.yticks(np.arange(0, 101, 10), **hfont)
    
    plt.rc('font', family='serif', size=16)
    plt.legend((p1[0], p2[0]), 
               ('Aliased', 'Non-aliased'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("AliasProportions_" + yearOfMeasurements + "_" + dateOfMeasurements + "_" + ASFilePath + ".pdf")
    plt.clf()
