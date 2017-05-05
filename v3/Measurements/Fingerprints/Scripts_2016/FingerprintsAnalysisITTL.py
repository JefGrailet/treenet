#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes stacked bar charts from the .fingerprint files collected during campaigns. The goal is 
# to evaluate the proportion of some types of fingerprints within a given AS. The script uses two 
# input parameters: a date (for the date of measurements) and the path to some text file listing 
# the ASes that should be considered at this date.
#
# This script focuses on initial TTL.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python FingerprintsAnalysisITTL.py [year] [date] [path to AS file]")
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
    ratio64 = []
    ratio128 = []
    ratio255 = []
    ratioOthers = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns" # TODO: edit this
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePath += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        dataFilePath += ".fingerprint"
        
        # Checks existence of the file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist")
            sys.exit()
        else:
            correctlyParsedASes.append(ASes[i])
        
        # Parses it and analyzes fingerprints
        with open(dataFilePath) as f:
            fingerprints = f.read().splitlines()
        
        nbFingerprints = len(fingerprints)
        integerData = np.zeros((4, 1))
        for j in range(0, nbFingerprints):
            firstSplit = fingerprints[j].split(' - ')
            curFingerprint = firstSplit[1][1:-1]
            splitted = curFingerprint.split(',')
            
            # Initial TTL
            if splitted[0] == "64":
                integerData[0] += 1
            elif splitted[0] == "128":
                integerData[1] += 1
            elif splitted[0] == "255":
                integerData[2] += 1
            else:
                integerData[3] += 1
        
        # Computes ratios
        ratio64.append((float(integerData[0]) / float(nbFingerprints)) * 100)
        ratio128.append((float(integerData[1]) / float(nbFingerprints)) * 100)
        ratio255.append((float(integerData[2]) / float(nbFingerprints)) * 100)
        ratioOthers.append((float(integerData[3]) / float(nbFingerprints)) * 100)

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
    
    bottom1 = ratio64
    bottom2 = np.zeros((len(correctlyParsedASes), 1))
    for i in range(0, len(correctlyParsedASes)):
        bottom2[i] = bottom1[i] + ratio128[i]
    bottom3 = np.zeros((len(correctlyParsedASes), 1))
    for i in range(0, len(correctlyParsedASes)):
        bottom3[i] = bottom2[i] + ratio255[i]

    plt.figure(figsize=(11,7))

    p1 = plt.bar(ind + padding, ratio64, width, color='#F0F0F0')
    p2 = plt.bar(ind + padding, ratio128, width, color='#D0D0D0', bottom=bottom1)
    p3 = plt.bar(ind + padding, ratio255, width, color='#888888', bottom=bottom2)
    p4 = plt.bar(ind + padding, ratioOthers, width, color='#000000', bottom=bottom3)
    
    plt.ylabel('Proportion (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,20])
    plt.xticks(ind + center, range(1,21,1), **hfont2)
    plt.yticks(np.arange(0, 101, 10), **hfont2)
    
    plt.rc('font', family='serif', size=15)
    plt.legend((p1[0], p2[0], p3[0], p4[0]), 
               ('64', '128', '255', '32'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("ITTL_" + yearOfMeasurements + "_" + dateOfMeasurements + "_" + ASFilePath + ".pdf")
    plt.clf()