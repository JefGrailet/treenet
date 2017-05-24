# About Measurements/ folder

*By Jean-François Grailet (last edited: May 24, 2017)*

## Overview

This folder contains measurements which were conducted from the PlanetLab testbed with `TreeNET` v3.0. Each sub-folder is named after a target Autonomous System (or AS) and contains measurements for that particular AS, with a sub-folder per dataset, denoted by the date at which the measurements for this dataset were  started.

At the root of each AS sub-folder, you should also find a "\[AS number\].txt" file, which contains the IP prefixes which are fed to `TreeNET` as the target prefixes of this AS.

## Typical target ASes

*Important remark (May 24):* target prefixes files have been reviewed because the announced prefixes for each AS contained redundant prefixes. This has no impact on the measurements of `TreeNET` since the program is designed to avoid duplicate subnets or duplicate entries in the IP dictionnary, but this means that the total amount of potential IPs per AS given in the TMA 2017 paper are larger than it should for several ASes. I apologize for this small mistake. Please use the table below to get the up-to-date total amounts of potential IPs per AS.

The next table lists the typical target ASes we measured with `TreeNET` and for which we provide our data in this repository.

|   AS    | AS name                  | Type    | Max. amount of IPs |
| :-----: | :----------------------- | :------ | :----------------- |
| AS109   | Cisco Systems            | Stub    | 1,173,760          |
| AS10010 | TOKAI Communications     | Transit | 1,445,376          |
| AS224   | UNINETT                  | Stub    | 1,115,392          |
| AS2764  | AAPT Limited             | Transit | 993,536            |
| AS5400  | British Telecom          | Transit | 1,385,216          |
| AS5511  | Orange S.A.              | Transit | 911,872            |
| AS6453  | TATA Communications      | Tier-1  | 656,640            |
| AS703   | Verizon Business         | Transit | 863,232            |
| AS8220  | COLT Technology          | Transit | 1,342,720          |
| AS8928  | Interoute Communications | Transit | 827,904            |
| AS12956 | Telefonica International | Tier-1  | 209,920            |
| AS13789 | Internap Network         | Transit | 96,256             |
| AS14    | University of Columbia   | Stub    | 339,968            |
| AS22652 | Fibrenoire, Inc.         | Transit | 76,288             |
| AS30781 | Jaguar Network           | Transit | 45,312             |
| AS37    | University of Maryland   | Stub    | 140,544            |
| AS4711  | INET Inc.                | Stub    | 17,408             |
| AS50673 | Serverius Holding        | Transit | 61,696             |
| AS52    | University of California | Stub    | 328,960            |
| AS802   | University of York       | Stub    | 71,936             |

## Composition of a dataset

Each dataset consists of 7 to 10 files:

* A subnet dump (.subnet), where subnets are listed with the following syntax:
 
  \[CIDR Notation\]\
  \[Classification\]\
  \[Responsive interfaces, as IP - TTL separated with ,\]\
  \[Traceroute, as IPs separated with ,\]\
  \[Blank line\]

* A dump of the IP dictionnary (.ip), a list of all responsive IPs with their respective minimum TTL value value to reach them from the vantage point and alias resolution hints (when available). The syntax is:
 
  IP - Minimum TTL: \[Alias resolution hints\]
 
  if this IP has alias resolution hints, otherwise it simply is
 
  IP - Minimum TTL

* **(2016 only)** A "*Bipartite ...*" file containing a double bipartite representation of the measured AS (one bipartite being router - subnet, the other being switch (L2) -- router), based on its representation as a network tree (not given, but can be obtained using either `TreeNET Reader` v2.3, either `TreeNET` v3.2 "*Architect*" or "*Forester*"). The bipartite file can be roughly split into 4 parts: the first lists the routers (and their labels), the second lists subnets (and their labels), the third provides links between routers of a same neighborhood with an imaginary switch and the last gives the links between routers and subnets, using their respective labels.

* **(Starting from 2017)** Two files, similar to the old "*Bipartite ...*" output file, suffixed with *.ers-graph* and *.ns-graph*. These output files present two distinct bipartite models of the dataset, with the *.ers-graph* file giving an output similar to the old "*Bipartite ...*" file while *.ns-graph* presents a simpler version where routers and imaginary switches are replaced by neighborhoods as seen in a network tree. These output files are exclusively produced by `TreeNET` v3.2 "*Architect*".

* **(Starting from April 2017)** Two files, ending with the extensions .tree and .neighborhoods, now also provide a text representation of the network tree and a in-depth analysis of the neighborhoods (respectively). The content of these files used to be part of the console output one would obtain by running any `TreeNET` version, but are now isolated in output files for convenience and spare viewers of this repository the effort of compiling and running `TreeNET` just to see the details of a network tree.

* A "*Statistics*" file giving some statistics on the subnets and the neighborhoods observed in the network tree obtained for this AS. Those can be re-obtained and analyzed in depth using `TreeNET Reader` v2.3 or `TreeNET` v3.2 "*Architect*".

* An .alias file which contains alias lists (one per line).

* A .fingerprint file which contains the IP fingerprints `TreeNET` computed for each IP involved in the alias resolution step. The main purpose of this file is to evaluate the overall behaviour of router interfaces in a given AS, and to evaluate if some alias resolution techniques are (still) relevant or outdated. See `TreeNET` v2.0 main README file to learn more about its fingerprinting method.

* A last file named "VP.txt" which gives the PlanetLab node from which `TreeNET` was run to get the corresponding dataset.

## Terminology used in the datasets

To better understand the datasets and the provided statistics, here are some definitions.

* **Accurate subnet:** a subnet which, having N responsive interfaces, features exactly N - 1 interfaces located at the same TTL (pivot interface) while the last one is located at that same TTL minus 1 (contra-pivot interface). From a topological point of view, the contra-pivot interface is the interface of the last router crossed before reaching the subnet that belongs to that subnet.

* **Odd subnet:** a subnet which features outliers preventing it from being classified as *accurate*. An *odd* subnet is not necessarily the result of a bad measurement, and can be the result of particularities in the target network architecture or routing policy.

* **Shadow subnet:** a subnet for which no contra-pivot could be found.

* **Alias Resolution Hint:** a notation consisting of:
  
  Initial TTL - token;ip-id,delay,token;ip-id,delay...token;ip-id,DNS
  
  The initial TTL is the inferred initial TTL of a reply packet sent by the corresponding IP. The possible values are: 32, 64, 128, 255. The inference consists in picking the closest upper value (i.e., if TTL in the reply is 45, its initial TTL is most likely 64; if it is 231, it will be 255, etc.).
  
  The amount of token;ip-id pairs (= N) can be chosen by the user, but it is typically 4 in most datasets. There are N - 1 delays between each pair, separated with a comma. The last comma delimits the last pair from the DNS of this IP, if it exists.
  
  The tokens are used to know in which order the ip-ids were retrieved. The delays express, in microseconds, the time that passed between the retrieval of each token;ip-id pair for a given IP. All these pieces of information are used to perform alias resolution by Ally or by studying the velocity of the IP-ID counter of this IP.
  
  The hint is also sometimes ended with a last part starting with "|" (with spaces). This part consists of either a "Yes" (i.e., this IP replies to ICMP timestamp requests), either an IP (source IP of the ICMP port unreachable message got after UDP probing), either both separated with a single comma.

* **Credible subnet:** a subnet which is either *accurate*, either *odd* with more than 70% of its interfaces being valid Pivot and less than 10% of its interfaces being Contra-Pivot candidate.

* **Neighborhood:** a location bordered by subnets which can reach each other with at most one hop. By extension, it is an internal node in the network tree produced by `TreeNET`.

* **Hedera:** the nickname given to (internal) multi-label nodes. "*Hedera*" is the Latin root of the French word "*lierre*", which denotes a kind of ivy. Ivies typically tying distinct branches together, it is a nice analogy for multi-label nodes which actually consist of several neighborhoods "tied" together into a single internal node.

* **Complete linkage:** denotes the fact that a neighborhood either has only leaves (i.e. subnets) as children, either has a subnet for each label of its children internal nodes (i.e. each subnet contains the label).
  
* **Partial linkage:** a relaxed definition of *complete* linkage. Either the neighborhood has *complete* linkage, either there lacks a single subnet for a single child internal.

## Notable remarks

* The .alias files only list the aliases themselves. The idea is that these files should be usable as input in software that has nothing to do with `TreeNET` but uses the same format for aliases (one alias per line, with one space between each IP). However, the aliases with the methods through which they were obtained can be found in the "*Bipartite...*" and .ers-graph files.

* Usually, datasets are *aligned* time-wise, i.e., the date at which measurements started (used to name the folders where you can find the data itself) is the same for all datasets. However, in January 2017, due to the unusually long time required for a few ASes (AS10010, AS2764 and AS8220), the other ASes were measured a few more times starting from January 15. This is why you will not find datasets for January 18, 25 and 27 in the folders of AS10010, AS2764 and AS8220.
