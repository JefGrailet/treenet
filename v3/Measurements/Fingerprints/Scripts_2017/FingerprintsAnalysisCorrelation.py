#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes stacked bar charts from the .fingerprint files collected during campaigns. The goal is 
# to evaluate the correlation that exists between some properties of the fingerprints. The script 
# uses three input parameters: a year, a date (both being used to select a dataset) and the path 
# to a text file listing the ASes that should be considered at this date.
#
# This script shows the proportions of fingerprints that have the following characteristics:
# 64 & "Healthy" IP-ID counter, 255 and "Echo" counter. Last category is for everything else.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python FingerprintsAnalysisCorrelation.py [year] [date] [path to AS file]")
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
    ratioHealthy64Or128 = []
    ratioEcho255 = []
    ratioOthers = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns"
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
        integerData = np.zeros((3, 1))
        for j in range(0, nbFingerprints):
            firstSplit = fingerprints[j].split(' - ')
            curFingerprint = firstSplit[1][1:-1]
            splitted = curFingerprint.split(',')
            
            # Counter type
            if splitted[2] == "Healthy" and (splitted[0] == "64" or splitted[0] == "128"):
                integerData[0] += 1
            elif splitted[2] == "Echo" and splitted[0] == "255":
                integerData[1] += 1
            else:
                integerData[2] += 1
        
        # Computes ratios
        ratioHealthy64Or128.append((float(integerData[0]) / float(nbFingerprints)) * 100)
        ratioEcho255.append((float(integerData[1]) / float(nbFingerprints)) * 100)
        ratioOthers.append((float(integerData[2]) / float(nbFingerprints)) * 100)

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
    
    bottom1 = ratioHealthy64Or128
    bottom2 = np.zeros((len(correctlyParsedASes), 1))
    for i in range(0, len(correctlyParsedASes)):
        bottom2[i] = bottom1[i] + ratioEcho255[i]

    plt.figure(figsize=(11,7))

    p1 = plt.bar(ind + padding, ratioHealthy64Or128, width, color='#F0F0F0')
    p2 = plt.bar(ind + padding, ratioEcho255, width, color='#D0D0D0', bottom=bottom1)
    p3 = plt.bar(ind + padding, ratioOthers, width, color='#888888', bottom=bottom2)
    
    plt.ylabel('Proportion (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,20])
    plt.xticks(ind + center, range(1,21,1), **hfont2)
    plt.yticks(np.arange(0, 101, 10), **hfont2)
    
    plt.rc('font', family='serif', size=15)
    plt.legend((p1[0], p2[0], p3[0]), 
               ('64/128, Healthy', '255, Echo', 'Others'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=3, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("Correlation_" + yearOfMeasurements + "_" + dateOfMeasurements + ".pdf")
    plt.clf()
