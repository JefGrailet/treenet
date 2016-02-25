#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Parses a subnet dump from TreeNET to make a quick analysis of the subnets (proportions per 
# prefix length, ratio of credible subnets) and the routes to evaluate how many of them are 
# complete (with the proportion of "repaired" routes), how many of them have 0.0.0.0 along 
# the way, how many end in 0.0.0.0 and how many have loops/cycles.

import numpy as np
import os
import sys

def formatSubnets(subnetsAsLines):
    """
    Formats a subnet dump (provided as list of lines) into a list of strings 
    containing:
    -[0]: subnet prefix,
    -[1]: prefix length,
    -[2]: classification by TreeNET (ACCURATE, ODD, SHADOW),
    -[3]: reponsive interfaces with TTLs (list of lists), 
    -[4]: interfaces along the route (as a list).

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
    currentSubnet = []
    inSubnetCounter = 0 
    for i in range(0, len(subnetsAsLines)):
        if not subnetsAsLines[i]:
            formatted.append(currentSubnet)
            currentSubnet = []
            inSubnetCounter = 0
        else:
            inSubnetCounter += 1
            
            # Dealing with prefix/prefix length
            if inSubnetCounter == 1:
                subnet = subnetsAsLines[i]
                splitted = subnet.split('/')
                currentSubnet.append(splitted[0])
                currentSubnet.append(splitted[1])
            
            # Status
            elif inSubnetCounter == 2:
                currentSubnet.append(subnetsAsLines[i])
            
            # Responsive interfaces
            elif inSubnetCounter == 3:
                line = subnetsAsLines[i]
                interfacesSplit = line.split(', ')
                nbInterfaces = len(interfacesSplit)
                
                interfaces = []
                for j in range(0, nbInterfaces):
                    withTTL = interfacesSplit[j].split(' - ')
                    interfaces.append(withTTL)
                
                currentSubnet.append(interfaces)
            
            # Route
            elif inSubnetCounter == 4:
                route = subnetsAsLines[i]
                splittedRoute = route.split(', ')
                currentSubnet.append(splittedRoute)

    return formatted

def isCredible(interfaces):
    """
    Provided a list of interfaces ("content" of the subnet) with their 
    respective minimum TTL, determines if it is a credible subnet (True) or 
    not (False).

    Parameters
    ----------
    interfaces : list of lists (fixed size: 2), n_interfaces
        The list of interfaces.
    
    Returns
    -------
    result : boolean value
        True if the subnet is credible
    
    """
    
    n_interfaces = len(interfaces)
    
    # First gets the minimum TTL
    minTTL = 255
    for i in range(0, n_interfaces):
        curTTL = int(interfaces[i][1])
        if curTTL < minTTL:
            minTTL = curTTL
    
    # See how interfaces are at minTTL and minTTL+1 + others
    interfacesAmount = np.zeros((3, 1))
    for i in range(0, n_interfaces):
        curTTL = int(interfaces[i][1])
        if curTTL == minTTL:
            interfacesAmount[0] += 1
        elif curTTL == (minTTL + 1):
            interfacesAmount[1] += 1
        else:
            interfacesAmount[2] += 1
    
    # Computes ratios
    ratioContraPivot = float(interfacesAmount[0]) / float(n_interfaces)
    ratioPivot = float(interfacesAmount[1]) / float(n_interfaces)
    ratioOutliers = float(interfacesAmount[2]) / float(n_interfaces)
    
    if ratioOutliers > 0:
        if ratioPivot >= 0.7 and ratioContraPivot <= 0.1:
            return True            
    else:
        if interfacesAmount[0] == 1 and interfacesAmount[1] > 0:
            return True
        elif ratioPivot > ratioContraPivot and (ratioPivot - ratioContraPivot) > 0.25:
            return True
    
    return False

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Please input a subnet dump (relative or absolute path)")
        sys.exit()
    
    dumpPath = str(sys.argv[1])
    
    # Gets subnets
    if not os.path.isfile(dumpPath):
        print("Subnet dump file does not exist")
        sys.exit()

    with open(dumpPath) as f:
        subnets = f.read().splitlines()
    
    formatted = formatSubnets(subnets)
    nbSubnets = len(formatted)
    
    # Statistics on the subnets
    coveredIPs = 0
    coveredIPsCredible = 0
    classifications = np.zeros((3, 1))
    subnetsAmounts = np.zeros((12, 1))
    credibleSubnets = np.zeros((12, 1))
    for i in range(0, nbSubnets):
        prefixLength = int(formatted[i][1])
        nbIPs = pow(2, 32 - prefixLength)
        
        index = prefixLength - 20
        subnetsAmounts[index] += 1
        coveredIPs += nbIPs
        
        if formatted[i][2] == "ACCURATE":
            classifications[0] += 1
            credibleSubnets[index] += 1
            coveredIPsCredible += nbIPs
        elif formatted[i][2] == "ODD":
            classifications[1] += 1
            if isCredible(formatted[i][3]):
                credibleSubnets[index] += 1
                coveredIPsCredible += nbIPs
        else:
            classifications[2] += 1
            # Rare occurrences of SHADOW subnets which should be re-classed
            if isCredible(formatted[i][3]):
                credibleSubnets[index] += 1
                coveredIPsCredible += nbIPs
    
    ratioAccurate = (float(classifications[0]) / float(nbSubnets)) * 100
    ratioOdd = (float(classifications[1]) / float(nbSubnets)) * 100
    ratioShadow = (float(classifications[2]) / float(nbSubnets)) * 100
    nbCredible = 0
    for i in range(0, 12):
        nbCredible += credibleSubnets[i]
    ratioCredible = (float(nbCredible) / float(nbSubnets)) * 100
    
    print("On subnet credibility...")
    print("ACCURATE subnets: " + str(ratioAccurate) + '%')
    print("ODD subnets: " + str(ratioOdd) + '%')
    print("SHADOW subnets: " + str(ratioShadow) + '%')
    print("Credible subnets: " + str(ratioCredible) + '%')
    print("Covered IPs: " + str(coveredIPs))
    print("Covered IPs (credible subnets): " + str(coveredIPsCredible))
    print("")
    
    print("Proportions of subnets per prefix length (+ credible):")
    for i in range(0, 12):
        prefix = "/" + str(i + 20)
        ratioPrefix = (float(subnetsAmounts[i]) / float(nbSubnets)) * 100
        credibleRatio = (float(credibleSubnets[i]) / float(nbSubnets)) * 100
        print(prefix + ": " + str(ratioPrefix) + "% (" + str(credibleRatio) + "%)")
    print("")
    
    # Statistics on the routes
    routeData = np.zeros((5, 1))
    for i in range(0, nbSubnets):
        sizeRoute = len(formatted[i][4])
        
        holes = 0
        repaired = False
        badEnd = False
        duplicate = 0
        tmpDict = dict()
        for j in range(0, sizeRoute):
            routeStep = formatted[i][4][j]
            
            # Repairment
            if routeStep.endswith("[Repaired]"):
                repaired = True
            
            # Route "holes" (i.e. 0.0.0.0)
            if routeStep == "0.0.0.0":
                holes += 1
                if j == (sizeRoute - 1):
                    badEnd = True
                    
            # Loops/cycles
            if routeStep not in tmpDict:
                tmpDict[routeStep] = 1
            else:
                duplicate += 1
        
        if holes == 0:
            routeData[0] += 1
        else:
            routeData[1] += 1
            if badEnd:
                routeData[2] += 1
        
        if repaired:
            routeData[3] += 1
        
        if duplicate > 1:
            routeData[4] += 1
    
    ratioComplete = (float(routeData[0]) / float(nbSubnets)) * 100
    ratioIncomplete = (float(routeData[1]) / float(nbSubnets)) * 100
    ratioBadEnd = (float(routeData[2]) / float(nbSubnets)) * 100
    ratioRepaired = (float(routeData[3]) / float(nbSubnets)) * 100
    ratioLoops = (float(routeData[4]) / float(nbSubnets)) * 100
    
    print("Out of " + str(nbSubnets) + " routes...")
    print("Ratio of complete routes: " + str(ratioComplete) + "%")
    print("Ratio of incomplete routes: " + str(ratioIncomplete) + "%")
    print("Ratio of routes ending in a hole: " + str(ratioBadEnd) + "%")
    print("Ratio of repaired routes: " + str(ratioRepaired) + "%")
    print("Ratio of routes with loops/cycles: " + str(ratioLoops) + "%")
