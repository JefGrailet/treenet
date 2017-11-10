#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes stacked bar charts from the various files (.ip, .fingerprint, .tree) of several 
# datasets collected by TreeNET v3.3.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python MultiLabelAliasResolution.py [year] [date] [path to AS file] [-n (optional) (ASes denoted by indexes)]")
        sys.exit()
    
    yearOfMeasurements = str(sys.argv[1])
    dateOfMeasurements = str(sys.argv[2])
    ASFilePath = str(sys.argv[3])
    
    rotatedLabels = True
    if len(sys.argv) == 5 and sys.argv[4] == '-n':
        rotatedLabels = False
    
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
    ratiosWithAliases = []
    ratiosFullyAliased = []
    ratiosAliased = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns/TreeNET"
    for i in range(0, len(ASes)):
        prefixPath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        prefixPath += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        
        dictPath = prefixPath + ".ip"
        treePath = prefixPath + ".tree"
        fingPath = prefixPath + ".fingerprint"
        
        # Gets IP dictionnary as lines
        if not os.path.isfile(dictPath):
            print("IP dictionnary file does not exist for " + ASes[i])
            continue

        with open(dictPath) as f:
            IPs = f.read().splitlines()
        
        # Parses the relevant lines to extract the pre-aliases
        aliases = []
        alreadySeen = set()
        for j in range(0, len(IPs)):
            if " || " in IPs[j]:
                firstSplit = IPs[j].split(" || ")
                preAliases = firstSplit[1]
                
                # Checks we don't already have seen this IP
                secondSplit = IPs[j].split(" - ")
                IP = secondSplit[0]
                if IP in alreadySeen:
                    continue
                alreadySeen.add(IP)
                
                alias = []
                alias.append(IP)
                if "," in preAliases:
                    thirdSplit = preAliases.split(",")
                    for k in range(0, len(thirdSplit)):
                        alias.append(thirdSplit[k])
                        alreadySeen.add(thirdSplit[k])
                else:
                    alias.append(preAliases)
                    alreadySeen.add(preAliases)
                alias.sort()
                aliases.append(alias)
        
        if len(aliases) == 0:
            print("No pre-alias discovered in the dataset of " + ASes[i] + ".")
            continue
        
        # Now gets the text output of the network tree as lines
        if not os.path.isfile(treePath):
            print("Tree dump file for " + ASes[i] + " does not exist.")
            continue

        with open(treePath) as f:
            treeLines = f.read().splitlines()
        
        # Parses the relevant lines to extract the multi-label nodes
        multiLabels = []
        inMultiLabel = set()
        for j in range(0, len(treeLines)):
            if "Hedera" in treeLines[j]:
                firstSplit = treeLines[j].split("Hedera: ")
                secondSplit = firstSplit[1].split(" (Previous:")
                multiLabelStr = secondSplit[0]
                multiLabelIPs = multiLabelStr.split(", ")
                multiLabels.append(multiLabelIPs)
                for k in range(0, len(multiLabelIPs)):
                    inMultiLabel.add(multiLabelIPs[k])
        
        # Remark: "hedera" is another name for a multi-label node ("hedera" = Latin root for ivy)
        
        # Now parses the fingerprints to see which IP can be aliased by UDP-based method/Ally
        if not os.path.isfile(fingPath):
            print("Fingerprint dump file of " + ASes[i] + " does not exist.")
            continue

        with open(fingPath) as f:
            fingerprints = f.read().splitlines()
        
        aliasable = set()
        for j in range(0, len(fingerprints)):
            splitted = fingerprints[j].split(',')
            if splitted[1] != '*' or splitted[2] == 'Healthy':
                splittedBis = fingerprints[j].split(' - ')
                if splittedBis[0] in inMultiLabel:
                    aliasable.add(splittedBis[0])
        
        # For each hedera, prints pre-aliases if they exist
        nbMultiLabels = len(multiLabels)
        nbRelevant = 0
        nbFullyCovered = 0
        sumAliasedIPs = 0
        sumAliasable = 0
        for j in range(0, nbMultiLabels):
            nbAliasable = 0
            for k in range(0, len(multiLabels[j])):
                if multiLabels[j][k] in aliasable:
                    nbAliasable += 1
            sumAliasable += nbAliasable
            
            relevant = []
            for k in range(0, len(aliases)):
                alias = aliases[k]
                if alias[0] in set(multiLabels[j]):
                    relevant.append(alias)
            
            if len(relevant) > 0:
                nbRelevant += 1
                
                nbAliasedIPs = 0
                for k in range(0, len(relevant)):
                    nbAliasedIPs += len(relevant[k])
                    
                sumAliasedIPs += nbAliasedIPs
                nInferredRouters = len(multiLabels[j]) - nbAliasedIPs + len(relevant)
                if nbAliasedIPs == nbAliasable:
                    nbFullyCovered += 1
        
        correctlyParsedASes.append(ASes[i])
        ratiosWithAliases.append((float(nbRelevant) / float(nbMultiLabels)) * 100)
        ratiosFullyAliased.append((float(nbFullyCovered) / float(nbMultiLabels)) * 100)
        ratiosAliased.append((float(sumAliasedIPs) / float(sumAliasable)) * 100)
    
    # Print ASes for which pre-aliases were discovered if -n is provided
    if not rotatedLabels:
        for i in range(0, len(correctlyParsedASes)):
            print(str(i + 1) + " = " + correctlyParsedASes[i])
    
    ind = np.arange(len(correctlyParsedASes)) # The x locations
    width = 0.3
    center = 0.5
    padding = 0.05
    
    # Font for labels and ticks
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':21}
    
    hfont2 = {'fontname':'serif',
             'fontsize':21}
    
    hfont3 = {'fontname':'serif',
             'fontsize':14}

    plt.figure(figsize=(11,7))

    p1 = plt.bar(ind + padding, ratiosWithAliases, width, color='#F0F0F0')
    p2 = plt.bar(ind + padding + width, ratiosFullyAliased, width, color='#888888')
    p3 = plt.bar(ind + padding + width * 2, ratiosAliased, width, color='#000000')
    
    plt.ylabel('Proportion (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,len(correctlyParsedASes)])
    if not rotatedLabels:
        plt.xticks(ind + center, range(1, len(correctlyParsedASes) + 1, 1), **hfont2)
    else:
        plt.xticks(ind + center, correctlyParsedASes, rotation=30, **hfont3)
        plt.gcf().subplots_adjust(bottom=0.15)
    plt.yticks(np.arange(0, 101, 10), **hfont2)
    
    plt.rc('font', family='serif', size=13)
    plt.legend((p1[0], p2[0], p3[0]), 
               ('Nodes w/ aliases', 'Fully aliased', 'Aliased/aliasable IPs'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("MultiLabelAnalysis_" + yearOfMeasurements + "_" + dateOfMeasurements + ".pdf")
    plt.clf()
