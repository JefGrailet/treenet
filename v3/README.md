# About TreeNET v3.0

*Last edited by Jean-François Grailet, Oct 10, 2016*
## Overview of the changes

### A full suite of software (Work In Progress)

Originally denoting a single piece of software, `TreeNET` will now denote three pieces of software. The first member of the trio, nicknamed *Arborist*, is the updated regular `TreeNET` used to measure a target network and output a router -- subnet topology from it.

The two other versions of `TreeNET` will be dedicated to second opinion measurement, dataset merging and dataset transformation for modeling purposes. These versions are still work in progress for now.

---

### Re-factored usage

The usage of `TreeNET` has been entirely re-factored in order to remove deprecated options/flags and provide a more consistent and more documented list of options and flags.

On a side note, `TreeNET` *Arborist* now provides an option to write the (unrefined) inference results of `ExploreNET` to an output file (ending in .xnet). This file provides, on each line, a subnet along the target IP from which it was infered and a few more details which were typically provided by the original `ExploreNET` (such as the alternate subnet, if it exists). This new option is useful to quantify how much the refinement phase improved the subnets and how efficient `ExploreNET` is.

---

### Re-factored verbosity and debug mode

Previous directly inherited from `ExploreNET` v2.1, the debug mode of `TreeNET` has been reviewed in-depth. `TreeNET` now provides several verbosity modes, with the most verbose mode being equivalent to debug. Each verbosity mode adds additionnal algorithmic details to the console output, with the debug adding logs detailling the different probes and their respective results. Mutual exclusion is used to display coherently each console message or debug log.

---

### Re-factored code

In addition to re-factoring some features which were neglected before, `TreeNET` v3.0 also comes with a re-organized source code which is specifically designed to add new ways of building and interpreting a *network tree* (i.e., the data structure used by `TreeNET` to model a target network after its subnets). These new algorithms will be added and made available during the course of the academic year 2016 -- 2017.

---

## Content of this folder

Currently, only `TreeNET` *Arborist* is publicly available, as other versions are still in the making. Its source code can be found in the *Arborist/* sub-folder.

A second sub-folder, *Measurements/*, provides datasets obtained by measuring ASes of varying sizes and roles from the PlanetLab testbed. There is a sub-folder for each target AS. Each of these folders provides a different sub-folder for each collected data set, denoted by the date at which the measurement started.

## Disclaimer

`TreeNET` v3.0 was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). Previous versions of `TreeNET` were written using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, 
currently assistant teacher at the University of Louisiana at Lafayette (USA).
The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be\
**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET`. I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
