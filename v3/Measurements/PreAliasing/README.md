# About PreAliasing/ folder

*By Jean-François Grailet (last edited: August 30, 2017)*

## Overview

This folder contains some Python script and an additionnal file to deal with the analysis of pre-alias resolution in TreeNET v3.3.

* **MultiLabelAliasResolution.py** is the script you are looking for. It parses three different types of file in each dataset (.ip, .tree and .fingerprint) in order to quantify how many multi-label nodes have aliases with their labels (proportionnally), how many of them have all their labels aliased together and how many aliasable IPs were aliased, and all that for each provided AS.

* **ASesList** is an example text file you can use as input in the Python script from above. It simply lists the ASes for which you want to generate the figure. The date of the datasets has to be provided in command line to the same script.
