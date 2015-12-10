#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Turns an output file from TreeNET/ExploreNET++ into a simplified list (i.e., it only displays 
# subnet prefix and prefix length along classification).

import numpy as np
import os
import sys

def formatSubnets(subnetsAsLines):
    """
    Formats a subnet dump (provided as list of lines) into a list of strings 
    displaying subnet prefix/prefix length and their respective classification 
    by TreeNET (ACCURATE, ODD, SHADOW).

    Parameters
    ----------
    subnetsAsLines : list, n_lines
        The subnet dump as a list of lines.
    
    Returns
    -------
    formatted : list of formatted subnets
        The formatted subnets (as described above)
    
    """

    formatted = []
    currentSubnet = ''
    inSubnetCounter = 0 
    for i in range(0, len(subnetsAsLines)):
        if not subnetsAsLines[i]:
            formatted.append(currentSubnet)
            currentSubnet = ''
            inSubnetCounter = 0
        else:
            inSubnetCounter += 1
            # Dealing with prefix/prefix length
            if inSubnetCounter == 1:
                currentSubnet = subnetsAsLines[i]
            # Status
            elif inSubnetCounter == 2:
                currentSubnet += ' ' + subnetsAsLines[i]

    return formatted

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Please input a subnet dump")
        sys.exit()
    
    dumpPath = './' + str(sys.argv[1])
    
    # Gets subnets
    if not os.path.isfile(dumpPath):
        print("Subnet dump file does not exist")
        sys.exit()

    with open(dumpPath) as f:
        subnets = f.read().splitlines()
    
    formatted = formatSubnets(subnets)
    
    # Writes new output
    splitted = dumpPath.split('/')
    splitted2 = splitted[len(splitted) - 1].split('.')
    outputName = splitted2[0] + '_simplified'

    if os.path.exists('./' + outputName):
        os.remove(outputName)
    with open(outputName, "a") as file:
        for i in range(0,len(formatted)):
            file.write(formatted[i] + '\n')
    
    print('Simplified dump has been written in ' + outputName)
