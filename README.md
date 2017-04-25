# TreeNET, a subnet-based topology discovery tool

*By Jean-François Grailet (last edited: April 14, 2017)*

## Overview

`TreeNET` is a subnet-based topology discovery tool which is built upon the subnet inference tool `ExploreNET`. Using the inference mechanics provided by `ExploreNET`, `TreeNET` refines the measured subnets and tries to infer a whole router - subnet topology on that basis. To reach this goal, it builds a tree-like structure which relies on `traceroute` measurements conducted to each measured subnet (using Paris `traceroute`).

The tree-like structure (called *network tree*) aims at highlighting *neighborhoods*, i.e., network locations bordered by subnets which can reach each other using a TTL value of at most 1 in IP packets. These locations typically consist of a single router or a mesh of several routers, possibly implemented with Layer-2 equipment. The knowledge of the subnets surrounding *neighborhoods* can be used to perform alias resolution and discover one or several routers and obtain a (near-)complete topology. Indeed, each subnet, if sound, should contain an interface with a particular distance in TTL value which necessarily belongs to the router giving access to that subnet. Listing such interfaces from all subnets bordering a same *neighborhood* is actually the first step of alias resolution in `TreeNET`.

## Version history (lattest version: v3.0)

All `TreeNET` versions are exclusively available for Linux and for the IPv4 protocol. There is currently no equivalent for IPv6.

* **`TreeNET` v1.0:** developped during academic year 2014-2015 for a master thesis, this version implements all major algorithmic steps described in the overview. This version also comes with measurements which were conducted with it from the PlanetLab testbed. It also comes with a companion piece of software (`TreeNET Reader`) which provides tools for analysis/merging of the collected datasets.

* **`TreeNET` v2.0 - v2.3:** elaborated in September 2015 and fully implemented as soon as November 2015 (as v2.0), this second version adds a new algorithmic step called "*network pre-scanning*" which aims at speeding up the overall execution. The alias resolution part has also been thoroughly improved, while other steps have been revisited for various but less consequent improvements.
  
  In January 2016, `TreeNET` v2.0 was upgraded to v2.1 with the introduction of new refinement mechanics which aims at speeding up the execution time and reducing the probing cost.
  
  In March 2016, `TreeNET` was further upgraded to v2.2. This version improves further the alias resolution step by reducing delays between the collected IP IDs (i.e., values of the *IP identifier* field on an IPv4 packet; such values are used by alias resolution tools like `Ally` and `MIDAR`) and detecting when remote IPs just repeat the IP IDs which were in the probe packets. It also now infers the initial TTL of ECHO reply packets. All the collected data is used to create a fingerprint of each IP. During the alias resolution process itself, `TreeNET` now groups IPs following their fingerprint (as IPs with similar behaviours should likely belong to a same device) and uses classical IP ID-based methods only when it is possible.
  
  In April 2016, `TreeNET` was upgraded again to v2.3, bringing new extensions to the alias resolution phase and the fingerprinting process, such as the classical address-based approach (used in `iffinder`).

* **`TreeNET` v3.0 - v3.2:** elaborated in Augustus 2016, this third version of `TreeNET` consists of a full suite of software with three major components. The first component, nicknamed *Arborist*, is available since October 2016 and is in practice the regular and improved `TreeNET`. It is essentially a major refactoring of the code and the options. Among others, it now provides several degrees of verbosity and the possibility to get the subnets as inferred by `ExploreNET` in an additionnal output file (optional).

  The second component, *Forester*, is available since November 18, 2016. It is a re-factored version of `TreeNET Reader`, minus the dataset transformation and analysis features, which are implemented in a third component (see below). *Forester*, in its current state, is dedicated to second opinion measurements (alias resolution and Paris `traceroute`) and dataset merging with a *grafting* algorithm (implemented, but not yet validated).
   
  The third and last component, *Architect*, was made available on December 21, 2016. It is a completely offline tool which is dedicated to the conversion of a dataset produced by `TreeNET` into several types of graph. `TreeNET Reader` already provided conversion of a tree into a double bipartite graph, but this feature was completely refactored in *Architect* and extended to be able to output several kinds of graph.
  
  On April 14, 2017, `TreeNET` was generally updated to its version v3.2. This version brings algorithmic improvements (in alias resolution and traceroute steps, in particular) as well as usage improvements.

## Related software
  
* `TreeNET` v2.0 was released along two additional pieces of software, `ARTest` and `ExploreNET++`. Both of them were implemented to assess precise steps of `TreeNET` v2.0: ARTest was made to assess the alias resolution step, while `ExploreNET++` was made to provide an input/output scheme for `ExploreNET` close to that of `TreeNET` in order to assess the refinement techniques introduced in `TreeNET`. Both of these variations were released at the same time as the initial v2.0.

* `TreeNET Reader`, the original companion software to `TreeNET` v1.0, has been upgraded to v2.0 in late January 2016, taking account of all improvements brought by `TreeNET` v2.0 and v2.1 and providing a few additionnal options and an improved bipartite conversion. It was upgraded to v2.1 in February 2016 to add a "*merging mode*" which merges datasets collected from different vantage points (or VPs) into a single dataset where routes to the different subnets are adapted to be able to build a single, coherent network tree, using new algorithms which require no additional probing. This feature, however, has not been validated yet and is currently not used in measurement campaigns. It is also available in `TreeNET` "*Forester*" v3.0, under a new name (*grafting mode*).
  
  In March of the same year, it was also upgraded to v2.2 at the same time as `TreeNET` "*Full*" to align on its lattest alias resolution improvements. The same scenario happened again the next month (April) with the relase of `TreeNET` v2.3.

## Content of this folder

As the names suggest, *v1/* contains all files related to `TreeNET` v1.0, while *v2/* and *v3/* provides all sources for `TreeNET` v2.3 and `TreeNET` v3.2 (respectively) along related software and datasets which were collected with it.

## Work in progress

`TreeNET` v3.2 is currently being used on PlanetLab to conduct new measurements. In particular, the collected traceroute data and its analysis by `TreeNET` will be studied in order to study traceroute anomalies and improve the construction of the network tree.

## Disclaimer

`TreeNET` and all related programs (`TreeNET Reader`, `ARTest`, `ExploreNET++`) were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
