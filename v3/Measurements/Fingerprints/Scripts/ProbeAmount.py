#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes a figure showing the amount of probes Ally, RadarGun, MIDAR and TreeNET will use to 
# conduct alias resolution. The amounts are not necessarily the exact amount that will be used, 
# they are more like predicted amounts that should however be faithful to experiment. The script 
# requires a .fingerprint and .ip file per AS to compute the right quantities.

import numpy as np
import os
import sys
import math
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python ProbeAmount.py [year] [date] [path to AS file]")
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
    predictionsAlly = []
    predictionsMIDAR = []
    predictionsTreeNET = []
    
    dataPath = "/home/jefgrailet/PhD/Campaigns" #TODO: edit this
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePath += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        
        # Checks existence of the file
        if not os.path.isfile(dataFilePath + ".ip") or not os.path.isfile(dataFilePath + ".fingerprint"):
            print(dataFilePath + ".ip and/or .fingerprint do not exist")
            sys.exit()
        else:
            correctlyParsedASes.append(ASes[i])
        
        # Parses and counts IP entries
        with open(dataFilePath + ".ip") as f:
            ipEntries = f.read().splitlines()
        nbIPs = len(ipEntries)
        
        # Parses and counts fingerprints
        with open(dataFilePath + ".fingerprint") as f:
            fingerprints = f.read().splitlines()
        nbFingerprints = len(fingerprints)

        # Computes predicted amounts
        predictionsAlly.append(nbIPs * nbIPs)
        predictionsMIDAR.append(30 * nbIPs)
        predictionsTreeNET.append(6 * nbFingerprints)

    ind = np.arange(len(correctlyParsedASes)) # The x locations
    width = 0.8
    center = 0.5
    padding = 0.1
    
    # Font for labels and ticks
    hfont = {'fontname':'serif',
             'fontsize':21}

    plt.figure(figsize=(11,7))

    p1 = plt.bar(ind + padding, predictionsAlly, width, color='#F0F0F0')
    p2 = plt.bar(ind + padding, predictionsMIDAR, width, color='#D0D0D0')
    p3 = plt.bar(ind + padding, predictionsTreeNET, width, color='#888888')
    
    plt.xlabel('AS index', **hfont)
    plt.xlim([0,20])
    plt.xticks(ind + center, range(1,21,1), **hfont)
    plt.ylabel('Amount of probes', **hfont)
    plt.yscale('log', nonposy='clip')
    
    plt.rc('font', family='serif', size=15)
    plt.legend((p1[0], p2[0], p3[0]), 
               ('Ally', 'MIDAR/RadarGun', 'TreeNET'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("ProbeAmount_" + yearOfMeasurements + "_" + dateOfMeasurements + "_" + ASFilePath + ".pdf")
    plt.clf()
