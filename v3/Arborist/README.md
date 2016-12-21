# About TreeNET v3.0.1 "Arborist" (treenet)

*By Jean-François Grailet, December 21, 2016*

## Compilation

You will need gcc and g++ on your Linux distribution to compile `TreeNET` (`treenet`). To compile it, set *Release/* as your working directory and execute the command:

```sh
make
```

If you need to recompile `TreeNET` after some editing, type the following commands:

```sh
make clean
make
```

## Deployement on PlanetLab testbed

If you intent to use `TreeNET` from the PlanetLab testbed, here is some advice.

* Do not bother with compiling `TreeNET` on PlanetLab nodes and rather compile it on your own computer. Then, you can upload the executable file (found in *Release/*) on a PlanetLab node and uses it as soon as you connect to it.

* Of course, your executable should be compiled with an environement similar to that of the PlanetLab nodes. The oldest OS you should find on a PlanetLab node is usually Fedora 8 (at the time this file was written). A safe (but slow) method to compile `TreeNET` for Fedora 8 and onwards is to run Fedora 8 as a virtual machine, put the sources on it, compile `TreeNET` and retrieve the executable file.

## Usage

`TreeNET` v3.0 "*Arborist*" will describe in details its options, flags and how you can use it by running the line:

```sh
./treenet -h
```

## Remarks

* Most machines forbid the user to open sockets to send probes, which prevents `TreeNET` from doing anything. To overcome this, run `TreeNET` as a super user (for example, with sudo).

* Most of the actual code of `TreeNET` is found in *src/treenet/*. *src/prober/* and *src/common/* provides libraries to handle (ICMP, UDP, TCP) probes, IPv4 addresses, etc. If you wish to build a completely different application using ICMP/UDP/TCP probing, you can take the full code of `TreeNET` and just remove the *src/treenet/* folder (same goes with `TreeNET` "*Forester*").

* Keep in mind, if you intend to remove or add files to the source code, that you have to edit the subdir.*mk* files in *Release/src/* accordingly. You should also edit *makefile* and sources.*mk* in *Release/* whenever you create a new (sub-)directory in *src/*.
  
* If you want to obtain all the details about subnet inference/refinement, inferred subnets and analysis of the tree, it is recommended to redirect the standard output to a file:
  
  ```sh
  sudo ./treenet v.w.x.y/z -l Test > Some_output_file.txt
  ```
  
  Note that the file might get quite big for a large amount of targets (several Megaoctets to dozens of Megaoctets). Keep the default verbosity (see usage with -h flag) to minimize the size of such file.

## Changes history

* **October 10, 2016:** first release of *Arborist* in its v3.0 version.

* **November 18, 2016:** first update of *Arborist*, now in version v3.0.1. Changes include additionnal re-factoring of the code and better exception handling to safely quit (and even save data collected so far) when a resource issue arises (e.g., failure to open a new socket).

* **December 21, 2016:** minor update of *Arborist*, fixing a light issue in the alias resolution part.

## Disclaimer

`TreeNET` v3 and its different versions were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
