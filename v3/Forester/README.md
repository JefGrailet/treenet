# About TreeNET v3.0 "Forester" (treenet_forester)

*By Jean-François Grailet, January 10, 2017*

## Compilation

You will need gcc and g++ on your Linux distribution to compile `TreeNET` "*Forester*" (`treenet_forester`). To compile it, set *Release/* as your working directory and execute the command:

```sh
make
```

If you need to recompile *Forester* after some editing, type the following commands:

```sh
make clean
make
```

## Deployement on PlanetLab testbed

If you intent to use `TreeNET` "*Forester*" from the PlanetLab testbed, here is some advice.

* Do not bother with compiling `TreeNET` "*Forester*" on PlanetLab nodes and rather compile it on your own computer. Then, you can upload the executable file (found in *Release/*) on a PlanetLab node and uses it as soon as you connect to it.

* Of course, your executable should be compiled with an environement similar to that of the PlanetLab nodes. The oldest OS you should find on a PlanetLab node is usually Fedora 8 (at the time this file was written). A safe (but slow) method to compile `TreeNET` "*Forester*" for Fedora 8 and onwards is to run Fedora 8 as a virtual machine, put the sources on it, compile `TreeNET` and retrieve the executable file.

## Usage

`TreeNET` v3.0 "*Forester*" will describe in details its options, flags and how you can use it by running the line:

```sh
./treenet_forester -h
```

## Remarks

* Most machines forbid the user to open sockets to send probes, which prevents *Forester* from doing second opinion probing work (like re-doing Paris `traceroute`). To overcome this, run *Forester* as a super user (for example, with sudo).

* Most of the actual code of *Forester* is found in *src/treenet/*. *src/prober/* and *src/common/* provides libraries to handle (ICMP, UDP, TCP) probes, IPv4 addresses, etc. If you wish to build a completely different application using ICMP/UDP/TCP probing, you can take the full code of *Forester* and just remove the *src/treenet/* folder (same goes with *Arborist*).

* Keep in mind, if you intend to remove or add files to the source code, that you have to edit the subdir.*mk* files in *Release/src/* accordingly. You should also edit *makefile* and sources.*mk* in *Release/* whenever you create a new (sub-)directory in *src/*.
  
* If you want to obtain all the details about probing work and/or analysis of the tree, it is recommended to redirect the standard output to a file:
  
  ```sh
  sudo ./treenet_forester Some_input_file -l Test -m 3 > Some_output_file.txt
  ```
  
  Note that the file might get quite big for large datasets (several Megaoctets to dozens of Megaoctets). Keep the default verbosity (see usage with -h flag) to minimize the size of such file.

## Changes history

* **November 18, 2016:** first release of *Forester*, as v3.0.

* **December 21, 2016:** minor update of *Forester*, fixing a light issue in the alias resolution part.

* **January 10, 2017:** minor update of *Forester*, fixing another light issue in the alias resolution part and updating a structure.

## Disclaimer

`TreeNET` v3 and its different versions were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
