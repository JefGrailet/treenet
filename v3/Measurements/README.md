# About Measurements/ folder

*By Jean-François Grailet, January 25, 2017*

## Overview

This folder contains measurements which were conducted from the PlanetLab testbed with `TreeNET` v3.0. Each sub-folder is named after a target Autonomous System (or AS) and contains measurements for that particular AS, with a sub-folder per dataset, denoted by the date at which the measurements for this dataset were  started.

At the root of each AS sub-folder, you should also find a "\[AS number\].txt" file, which contains the IP prefixes which are fed to `TreeNET` as the target prefixes of this AS.

## Composition of a dataset

Each dataset consists of 7 to 8 files:

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

* **(2016 only)** A "*Bipartite ...*" file containing a double bipartite representation of the measured AS (one bipartite being router - subnet, the other being switch (L2) -- router), based on its representation as a network tree (not given, but can be obtained using either `TreeNET Reader` v2.3, either `TreeNET` v3.0 "*Architect*" or "*Forester*"). The bipartite file can be roughly split into 4 parts: the first lists the routers (and their labels), the second lists subnets (and their labels), the third provides links between routers of a same neighborhood with an imaginary switch and the last gives the links between routers and subnets, using their respective labels.

* **(Starting from 2017)** Two files, similar to the old "*Bipartite ...*" output file, suffixed with *.ers-graph* and *.ns-graph*. These output files present two distinct bipartite models of the dataset, with the *.ers-graph* file giving an output similar to the old "*Bipartite ...*" file while *.ns-graph* presents a simpler version where routers and imaginary switches are replaced by neighborhoods as seen in a network tree. These output files are exclusively produced by `TreeNET` v3.0 "*Architect*".

* A "*Statistics*" file giving some statistics on the subnets and the neighborhoods observed in the network tree obtained for this AS. Those can be re-obtained and analyzed in depth using `TreeNET Reader` v2.3 or `TreeNET` v3.0 "*Architect*".

* An .alias file which contains alias lists (one per line).

* A .fingerprint file which contains the IP fingerprints `TreeNET` computed for each IP involved in the alias resolution step. The main purpose of this file is to evaluate the overall behaviour of router interfaces in a given AS, and to evaluate if some alias resolution techniques are (still) relevant or outdated. See `TreeNET` v2.0 main README file to learn more about its fingerprinting method.

* A last file named "VP.txt" which gives the PlanetLab node from which `TreeNET` was run to get the corresponding dataset.

## Terminology used in the datasets

To better understand the datasets and the provided statistics, here are some definitions.

* **Accurate subnet:** a subnet which, having N responsive interfaces, features exactly N - 1 interfaces located at the same TTL (Pivot interface) while the last one is located at that same TTL minus 1 (Contra-Pivot interface). From a topological point of view, the Contra-Pivot interface is the interface of the last router crossed before reaching the subnet that belongs to that subnet.

* **Odd subnet:** a subnet which features outliers preventing it from being classified as *accurate*. An *odd* subnet is not necessarily the result of a bad measurement, and can be the result of particularities in the target network architecture or routing policy.

* **Shadow subnet:** a subnet for which no Contra-Pivot could be found.

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
