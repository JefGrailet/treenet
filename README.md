# TreeNET, a subnet-based topology discovery tool

*By Jean-François Grailet (last edited: August 18, 2017)*

## Overview

`TreeNET` is a subnet-based topology discovery tool which is built upon the subnet inference tool `ExploreNET`. Using the inference mechanics provided by `ExploreNET`, `TreeNET` refines the measured subnets and tries to infer a whole router - subnet topology on that basis. To reach this goal, it builds a tree-like structure which relies on `traceroute` measurements conducted to each measured subnet (using Paris `traceroute`).

The tree-like structure (called *network tree*) aims at highlighting *neighborhoods*, i.e., network locations bordered by subnets which can reach each other using a TTL value of at most 1 in IP packets. These locations typically consist of a single router or a mesh of several routers, possibly implemented with Layer-2 equipment. The knowledge of the subnets surrounding *neighborhoods* can be used to perform alias resolution and discover one or several routers and obtain a (near-)complete topology. Indeed, each subnet, if sound, should contain an interface with a particular distance in TTL value which necessarily belongs to the router giving access to that subnet. Listing such interfaces from all subnets bordering a same *neighborhood* is actually the first step of alias resolution in `TreeNET`.

## Publications

`TreeNET` and its measurements have been the topics of several publications. People wishing to get a *big picture* on the software free of implementation details are encouraged to read them.

* [TreeNET: Discovering and Connecting Subnets](http://www.run.montefiore.ulg.ac.be/~grailet/docs/publications/TreeNET_TMA_2016.pdf)
  Jean-François Grailet, Fabien Tarissan, Benoit Donnet
  [Traffic Monitoring and Analysis Workshop (TMA) 2016](http://tma.ifip.org/2016/), Louvain-La-Neuve, 07/04/2016 - 08/04/2016

* [Towards a Renewed Alias Resolution with Space Search Reduction and IP Fingerprinting](http://www.run.montefiore.ulg.ac.be/~grailet/docs/publications/Alias_Resolution_TMA_2017.pdf)
  Jean-François Grailet, Benoit Donnet
  [Network Traffic Measurement and Analysis Conference (TMA) 2017](http://tma.ifip.org/2017/), Dublin, 21/06/2017 - 23/06/2017

## Version history (lattest version: v3.2)

All `TreeNET` versions are exclusively available for Linux and for the IPv4 protocol. There is currently no equivalent for IPv6.

* **`TreeNET` v1.0:** developped during academic year 2014-2015 for a master thesis, this version implements all major algorithmic steps described in the overview. This version also comes with measurements which were conducted with it from the PlanetLab testbed. It also comes with a companion piece of software (`TreeNET Reader`) which provides additionnal tools for the analysis and merging of the collected datasets.

* **`TreeNET` v2.0 - v2.3:** elaborated in September 2015 and fully implemented as soon as November 2015 (as v2.0), this second version adds a new algorithmic step called "*network pre-scanning*" which aims at speeding up the overall execution. The alias resolution part has also been thoroughly improved, while other steps have been revisited for various but less consequent improvements. It was subsequently upgraded up to v2.3 (April 2016) to speed it up a bit more and also to further deepen alias resolution, notably by introducing a fingerprinting process to study the viability of different state-of-the-art alias resolution techniques and pick the most suited method depending on the context.

* **`TreeNET` v3.0 - v3.2:** elaborated in August 2016, this third version of `TreeNET` consists of a full suite of software with three major components. The first component, nicknamed *Arborist*, is available since October 2016 and is in practice the regular and improved `TreeNET`. It is essentially a major refactoring of the code and the options. Among others, it now provides several degrees of verbosity and the possibility to get the subnets as inferred by `ExploreNET` in an additionnal output file (optional). The two other components, *Forester* and *Architect*, were made available respectively in November and December 2016 and are essentially a re-factoring of `TreeNET Reader`, though *Architect* deepened a lot the graph interpretation of a network tree: it is indeed able to output 5 different types of graph, while `TreeNET Reader` could only output one type.

## Related software
  
* `TreeNET` v2.0 was released along two additional pieces of software, `ARTest` and `ExploreNET++`. Both of them were implemented to assess precise steps of `TreeNET` v2.0: ARTest was made to assess the alias resolution step, while `ExploreNET++` was made to provide an input/output scheme for `ExploreNET` close to that of `TreeNET` in order to assess the refinement techniques introduced in `TreeNET`. Both of these variations were released at the same time as the initial v2.0 and are also available in this repository.

* `TreeNET Reader`, the original companion software to `TreeNET` v1.0, further upgraded to v2.0 in late January 2016, is a software designed to parse and analyze output files produced by `TreeNET`. It notably implements a bipartite conversion of network trees, and an experimental method to merge network trees obtained from distinct vantage points (VPs). These features still exist now in `TreeNET` "*Forester*" and `TreeNET` "*Architect*", first released respectively in November and December 2016, and have been deepened since. The original versions of `TreeNET Reader` are however still available in this repository.

## Content of this folder

As the names suggest, *v1/* contains all files related to `TreeNET` v1.0, while *v2/* and *v3/* provides all sources for `TreeNET` v2.3 and `TreeNET` v3.2 (respectively) along related software and datasets which were collected with it.

## Work in progress

`TreeNET` will be upgraded to v3.3 in the coming months in order to improve a bit more the alias resolution step. `traceroute` anomalies are still being studied in the meantime.

## Disclaimer

`TreeNET` and all related programs (`TreeNET Reader`, `ARTest`, `ExploreNET++`) were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
