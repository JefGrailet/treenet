#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys

def getCapacity(prefix):

    prefixComponents = prefix.split('/')
    if len(prefixComponents) == 1:
        return 1
    
    prefixLength = int(prefixComponents[1])

    return pow(2, 32 - prefixLength)

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Please input a target file (IP ranges/single IP, one per line)")
        sys.exit()
    
    targetFilePath = './' + str(sys.argv[1])
    
    # Retrieving data
    if not os.path.isfile(targetFilePath):
        print("Submitted file does not exist")
        sys.exit()

    with open(targetFilePath) as f:
        targets = f.read().splitlines()

    capacity = 0
    for i in range(0, len(targets)):
        capacity += getCapacity(targets[i])
    
    print("Total amount of IPs: " + str(capacity))
