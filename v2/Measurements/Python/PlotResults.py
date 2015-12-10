#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Takes the .plot output file of TreeNET "ARTest" (Alias Resolution assessment version) and plots 
# the data it contains first as a probability density function (PDF), then as a cumulative density 
# function (CDF).

import numpy as np
import os
import sys
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Please input a .plot file (output from TreeNET ARTest)")
        sys.exit()
    
    dataFilePath = './' + str(sys.argv[1])
    
    # Retrieving data
    if not os.path.isfile(dataFilePath):
        print("Submitted file does not exist")
        sys.exit()

    with open(dataFilePath) as f:
        plotData = f.read().splitlines()
    
    # Counts the occurrences of each value
    occurrences = []
    values = []
    i = 0
    impossibleCases = 0
    while i < len(plotData):
        cur = plotData[i]
        if cur == '-1':
            i += 1
            impossibleCases += 1
            continue
        
        occ = 1
        i += 1
        while i < len(plotData) and plotData[i] == cur:
            occ += 1
            i += 1
        
        occurrences.append(occ)
        values.append(cur)
    
    processedOcc = np.array(occurrences)
    processedVal = np.array(values)
    
    N = len(plotData)
    M = len(processedOcc)
    
    percentage = (float(impossibleCases) / float(N)) * 100
    print("Percentage of alias lists which were fully unresponsive: " + str(percentage) + "%")
    print("Those will not be taken into account in the PDF/CDF plots.")
    
    # Probability Density Function
    proba = []
    for i in range(0, M):
        proba.append(float(processedOcc[i]) / (float(N) - impossibleCases))
    
    processedProba = np.array(proba)
    
    # Cumulative Density Function
    cumul = []
    for i in range(0, M):
        if i == 0:
            cumul.append(processedProba[i])
        else:
            cumul.append(cumul[i - 1] + processedProba[i])
    
    processedCumul = np.array(cumul)
    
    # For the name of the output file, we simply keep the same name, minus .plot
    exploded1 = dataFilePath.split("/")
    exploded2 = exploded1[len(exploded1) - 1].split(".")
    plotFileName = exploded2[0]
    
    # Plots results
    hfont = {'fontname':'serif',
             'fontsize':32}
    
    hfontTicks = {'fontname':'serif',
                  'fontsize':24}
    
    plt.figure(figsize=(12,9))
    
    plt.plot(processedVal, processedProba, color='black', linewidth=3)
    plt.xticks(np.arange(0, 1.1, 0.1), **hfontTicks)
    plt.yticks(np.arange(0, 1.1, 0.1), **hfontTicks)
    ax = plt.axes()
    yticks = ax.yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    plt.xlabel("Accuracy", **hfont)
    plt.ylabel("Probability Density Function", **hfont)
    plt.grid()
    plt.savefig("./" + plotFileName + "_PDF.pdf")
    plt.clf()
    
    plt.figure(figsize=(12,9))
    
    plt.plot(processedVal, processedCumul, color='black', linewidth=3)
    plt.xticks(np.arange(0, 1.1, 0.1), **hfontTicks)
    plt.yticks(np.arange(0, 1.1, 0.1), **hfontTicks)
    ax = plt.axes()
    yticks = ax.yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    plt.xlabel("Accuracy", **hfont)
    plt.ylabel("Cumulative Density Function", **hfont)
    plt.grid()
    plt.savefig("./" + plotFileName + "_CDF.pdf")
    plt.clf()
