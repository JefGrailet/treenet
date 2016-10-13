#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes stacked bar charts from the .fingerprint files collected during campaigns. The goal is 
# to evaluate the proportion of some types of fingerprints within a given AS. The script uses two 
# input parameters: a date (for the date of measurements) and the path to some text file listing 
# the ASes that should be considered at this date.
#
# This script focuses on the ratio between fingerprinted IPs and responsive IPs.

import numpy as np
import os
import sys
from matplotlib import pyplot as plt
from numpy import arange

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python FingerprintsAnalysisSSR.py [year] [date] [path to AS file]")
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
        
    # For this particular file, we expect a format AS:type:largestList, but ignoring type.
    ASes = []
    LargestLists = []
    for i in range(0, len(ASesRaw)):
        splitted = ASesRaw[i].split(':')
        ASes.append(splitted[0])
        LargestLists.append(splitted[2])

    # Computes the required data
    correctlyParsedASes = []
    ratioFingerprinted = []
    ratioWithLargestList = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns" # TODO: edit this
    for i in range(0, len(ASes)):
        dataFilePrefix = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePrefix += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        
        fingerprintsPath = dataFilePrefix + ".fingerprint"
        ipDictPath = dataFilePrefix + ".ip"
        
        # Checks existence of both files file
        if not os.path.isfile(fingerprintsPath):
            print(fingerprintsPath + " does not exist")
            sys.exit()
        
        if not os.path.isfile(ipDictPath):
            print(ipDictPath + " does not exist")
            sys.exit()
        
        correctlyParsedASes.append(ASes[i])
        
        # Parses both files to count lines
        with open(fingerprintsPath) as f:
            fingerprints = f.read().splitlines()
        
        with open(ipDictPath) as f:
            ipDictLines = f.read().splitlines()
        
        # Computes ratio
        ratioFingerprinted.append((float(len(fingerprints)) / float(len(ipDictLines))) * 100)
        ratioWithLargestList.append((float(LargestLists[i]) / float(len(ipDictLines))) * 100)

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

    p1 = plt.bar(ind + padding, ratioFingerprinted, width, color='#AAAAAA')
    p2 = plt.plot(arange(0.5,20.5,1.0), ratioWithLargestList, color='#000000', linewidth=3)
    
    plt.ylabel('Ratio (%)', **hfont)
    plt.xlabel('AS index', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,20])
    plt.xticks(ind + center, range(1,21,1), **hfont2)
    plt.yticks(np.arange(0, 101, 10), **hfont2)
    
    plt.rc('font', family='serif', size=15)
    plt.legend((p1[0], p2[0]), 
               ('Fingerprinted IPs', 'Largest aggregate'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("SSR_" + yearOfMeasurements + "_" + dateOfMeasurements + "_" + ASFilePath + ".pdf")
    plt.clf()
