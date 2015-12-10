#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Parses a subnet dump from TreeNET for a quick analysis with covered IPs and 
# the proportion of each class of subnets.

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

    if len(sys.argv) < 2:
        print("Please input the TreeNET subnet dump")
        sys.exit()
    
    treenetDumpPath = './' + str(sys.argv[1])
    
    # Gets subnets outputted by TreeNET
    if not os.path.isfile(treenetDumpPath):
        print("TreeNET subnet dump file does not exist")
        sys.exit()

    with open(treenetDumpPath) as f:
        treenetSubnets = f.read().splitlines()
    
    treenetFormatted = formatSubnets(treenetSubnets)
    
    # Covered IPs
    treenetCoverage = 0
    for i in range(0, len(treenetFormatted)):
        treenetCoverage += treenetFormatted[i][1]
    
    # Percentages of ACCURATE/ODD/SHADOW subnets
    percentagesTreenet = np.array([0.0, 0.0, 0.0])
    for i in range(0, len(treenetFormatted)):
        percentagesTreenet[treenetFormatted[i][2] - 1] += 1
    
    for i in range(0, 3):
        percentagesTreenet[i] /= len(treenetFormatted)
    
    print("About subnets inferred by TreeNET (" + str(len(treenetFormatted)) + "):")
    print("Covered IPs: " + str(treenetCoverage))
    print("ACCURATE subnets: " + str(percentagesTreenet[0] * 100) + "%")
    print("ODD subnets: " + str(percentagesTreenet[1] * 100) + "%")
    print("SHADOW subnets: " + str(percentagesTreenet[2] * 100) + "%")
