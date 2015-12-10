#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Compares output files from TreeNET and ExploreNET to assess the efficience of the algorithms 
# added in TreeNET for subnet inference/refinement. The file outputs the percentages of accurate, 
# odd and incomplete subnets for each (+ shadow for TreeNET), then computes the percentages of 
# subnets found by ExploreNET which:
# -are redundant (i.e. 2 distinct target addresses lead to the same inferred subnet)
# -are encompassed in larger subnets found by TreeNET.
# It should be noted that the version of ExploreNET which produces the output expected by this 
# short script is not v2.1 but ExploreNET++, a slightly revisited version of ExploreNET v2.1 
# which features (among others) parallelism similar to that of TreeNET.

import numpy as np
import os
import sys

def formatSubnets(subnetsAsLines):
    """
    Formats a subnet dump (provided as list of lines) into an array of 
    triplets (base IP,size,status) where:
    -base IP is the prefix, as a long int,
    -size is the amount of possible IPs in this subnet,
    -status is the label for this subnet, as an integer (0 = INCOMPLETE, 
     1 = ACCURATE, 2 = ODD, 3 = SHADOW)

    Parameters
    ----------
    subnetsAsLines : list, n_lines
        The subnet dump as a list of lines.
    
    Returns
    -------
    formatted : array of shape [n_subnets, 3]
        The formatted subnets (as described above)
    
    """

    formatted = []
    currentSubnet = []
    subnetCounter = 0
    inSubnetCounter = 0 
    for i in range(0, len(subnetsAsLines)):
        if not subnetsAsLines[i]:
            formatted.append(currentSubnet)
            currentSubnet = []
            subnetCounter += 1
            inSubnetCounter = 0
        else:
            inSubnetCounter += 1
            # Dealing with prefix/prefix length
            if inSubnetCounter == 1:
                subnetSplitted = subnetsAsLines[i].split("/")
                
                prefix = subnetSplitted[0].split(".")
                prefixLong = 256 * 256 * 256 * int(prefix[0])
                prefixLong += 256 * 256 * int(prefix[1])
                prefixLong += 256 * int(prefix[2])
                prefixLong += int(prefix[3])
                
                currentSubnet.append(prefixLong)
                
                size = pow(2, 32 - int(subnetSplitted[1]))
                
                currentSubnet.append(size)
            
            # Status
            elif inSubnetCounter == 2:
                if subnetsAsLines[i] == 'ACCURATE':
                    currentSubnet.append(1)
                elif subnetsAsLines[i] == 'ODD':
                    currentSubnet.append(2)
                elif subnetsAsLines[i] == 'SHADOW':
                    currentSubnet.append(3)
                else:
                    currentSubnet.append(0)

    return np.array(formatted)

if __name__ == "__main__":

    if len(sys.argv) < 3:
        print("Please input the TreeNET and ExploreNET subnet dumps (respectively)")
        sys.exit()
    
    treenetDumpPath = './' + str(sys.argv[1])
    explorenetDumpPath = './' + str(sys.argv[2])
    
    # Gets subnets outputted by TreeNET
    if not os.path.isfile(treenetDumpPath):
        print("TreeNET subnet dump file does not exist")
        sys.exit()

    with open(treenetDumpPath) as f:
        treenetSubnets = f.read().splitlines()
    
    # Gets subnets outputted by ExploreNET
    if not os.path.isfile(explorenetDumpPath):
        print("ExploreNET subnet dump file does not exist")
        sys.exit()

    with open(explorenetDumpPath) as f:
        explorenetSubnets = f.read().splitlines()
    
    treenetFormatted = formatSubnets(treenetSubnets)
    explorenetFormatted = formatSubnets(explorenetSubnets)
    
    # Covered IPs
    treenetCoverage = 0
    explorenetCoverage = 0
    for i in range(0, len(treenetFormatted)):
        treenetCoverage += treenetFormatted[i][1]
    
    # For covered IPs for ExploreNET, redundant subnets must be avoided
    previous = None
    for i in range(0, len(explorenetFormatted)):
        if previous != None:
            current = explorenetFormatted[i]
            if previous[0] != current[0] and previous[1] != current[1]:
                explorenetCoverage += explorenetFormatted[i][1]
            
        previous = explorenetFormatted[i]
    
    # Percentages of ACCURATE/ODD/SHADOW subnets (TreeNET)
    percentagesTreenet = np.array([0.0, 0.0, 0.0])
    for i in range(0, len(treenetFormatted)):
        percentagesTreenet[treenetFormatted[i][2] - 1] += 1
    
    for i in range(0, 3):
        percentagesTreenet[i] /= len(treenetFormatted)
    
    print("About subnets inferred by TreeNET (" + str(len(treenetFormatted)) + "):")
    print("Covered IPs: " + str(treenetCoverage))
    print("ACCURATE subnets: " + str(percentagesTreenet[0] * 100) + "%")
    print("ODD subnets: " + str(percentagesTreenet[1] * 100) + "%")
    print("SHADOW subnets: " + str(percentagesTreenet[2] * 100) + "%\n")
    
    # Percentages of INCOMPLETE/ACCURATE/ODD subnets (ExploreNET)
    percentagesExplorenet = np.array([0.0, 0.0, 0.0])
    for i in range(0, len(explorenetFormatted)):
        percentagesExplorenet[explorenetFormatted[i][2]] += 1
    
    for i in range(0, 3):
        percentagesExplorenet[i] /= len(explorenetFormatted)
    
    print("About subnets inferred by ExploreNET (" + str(len(explorenetFormatted)) + "):")
    print("Covered IPs: " + str(explorenetCoverage))
    print("INCOMPLETE subnets: " + str(percentagesExplorenet[0] * 100) + "%")
    print("ACCURATE subnets: " + str(percentagesExplorenet[1] * 100) + "%")
    
    # Percentage of redundant subnets in ExploreNET. For faster computation, it is assumed that 
    # the subnets were originally sorted by prefix (so similar prefix are consecutive), which is 
    # always the case with subnet dumps from TreeNET or ExploreNET++.
    
    redundant = 0.0
    previous = None
    for i in range(0, len(explorenetFormatted)):
        if previous != None:
            current = explorenetFormatted[i]
            if previous[0] == current[0] and previous[1] == current[1]:
                redundant += 1
            
        previous = explorenetFormatted[i]
    
    redundant /= len(explorenetFormatted)
    print("Redundant subnets: " + str(redundant * 100) + "%")
    
    # Percentage of subnets from ExploreNET encompassed in subnets from TreeNET. If a subnet found 
    # by ExploreNET is the same size as the one found by TreeNET, it is not counted as encompassed.
    
    encompassed = 0.0
    for i in range(0, len(treenetFormatted)):
        lowBound = treenetFormatted[i][0]
        upBound = treenetFormatted[i][0] + treenetFormatted[i][1]
        
        for j in range(0, len(explorenetFormatted)):
            curLowBound = explorenetFormatted[j][0]
            curUpBound = explorenetFormatted[j][0] + explorenetFormatted[j][1]
            
            if lowBound <= curLowBound and upBound >= curUpBound:
                if lowBound != curLowBound or upBound != curUpBound:
                    encompassed += 1
    
    encompassed /= len(explorenetFormatted)
    print("Subnets encompassed in subnets found by TreeNET: " + str(encompassed * 100) + "%")
