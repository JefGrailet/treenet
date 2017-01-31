#! /usr/bin/env python
# -*- coding: utf-8 -*-

# This script evaluates, for each dataset:
# -the ratio between the total of fingerprints found in a dataset with respect to the total amount 
#  of responsive IPs during the measurement (with a bar chart),
# -the ratio between the largest fingerprint list (in a single neighborhood) found within the 
#  network tree with respect to the total amount of fingerprints (with a simple curve).
# The goal of such a figure is to evaluate the benefits of the space search reduction. To this 
# end, the script consults three files from each dataset:
# -IP dictionnary,
# -.fingerprint output file,
# -statistics file (gives the size of the largest fingerprint aggregate).

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
        
    # For this particular file, we do not class by type. We remove the :[type] part.
    ASes = []
    for i in range(0, len(ASesRaw)):
        splitted = ASesRaw[i].split(':')
        ASes.append(splitted[0])

    # Computes the required data
    correctlyParsedASes = []
    ratioFingerprinted = []
    ratioWithLargestList = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns"
    for i in range(0, len(ASes)):
        dataFilePrefix = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePrefix += dateOfMeasurements + "/"
        
        fingerprintsPath = dataFilePrefix + ASes[i] + "_" + dateOfMeasurements + ".fingerprint"
        ipDictPath = dataFilePrefix + ASes[i] + "_" + dateOfMeasurements + ".ip"
        statsPath = dataFilePrefix + "Statistics " + ASes[i] + "_" + dateOfMeasurements
        
        # Checks existence of all files
        if not os.path.isfile(fingerprintsPath):
            print(fingerprintsPath + " does not exist")
            sys.exit()
        
        if not os.path.isfile(ipDictPath):
            print(ipDictPath + " does not exist")
            sys.exit()
        
        if not os.path.isfile(statsPath):
            print(statsPath + " does not exist")
            sys.exit()
        
        correctlyParsedASes.append(ASes[i])
        
        # Parses all files into lines
        with open(fingerprintsPath) as f:
            fingerprints = f.read().splitlines()
        
        with open(ipDictPath) as f:
            ipDictLines = f.read().splitlines()
        
        with open(statsPath) as f:
            statsLines = f.read().splitlines()
        
        # Largest fingerprint aggregate is given at the last line of the stats file
        lineLargestList = statsLines[len(statsLines) - 1]
        lineSplitted = lineLargestList.split(': ')
        largestList = int(lineSplitted[1])
        
        # Computes ratio
        ratioFingerprinted.append((float(len(fingerprints)) / float(len(ipDictLines))) * 100)
        ratioWithLargestList.append((float(largestList) / float(len(ipDictLines))) * 100)

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
    
    plt.ylabel('Ratio (%) of responsive IPs', **hfont)
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

    plt.savefig("SSR_" + yearOfMeasurements + "_" + dateOfMeasurements + ".pdf")
    plt.clf()
