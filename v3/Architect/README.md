# About TreeNET v3.0 "Architect" (treenet_architect)

*By Jean-François Grailet, December 21, 2016*

## Compilation

You will need gcc and g++ on your Linux distribution to compile `TreeNET` "*Architect*" (`treenet_architect`). To compile it, set *Release/* as your working directory and execute the command:

```sh
make
```

If you need to recompile *Architect* after some editing, type the following commands:

```sh
make clean
make
```

## Usage

`TreeNET` v3.0 "*Architect*" will describe in details its options, flags and how you can use it by running the line:

```sh
./treenet_forester -h
```

## Remarks

* It is possible that a few nodes in a (double) bipartite graph produced by *Architect* will not be connected to the graph (which should always consist of a single connected component). This is a rare occurrence that will be corrected in the next releases.

* Unlike *Arborist* and *Forester*, *Architect* is a completely passive tool that will not conduct any measurement of any kind. It is solely dedicated to the conversion of datasets into graphs.

* Most of the actual code of *Architect* is found in *src/treenet/*. *src/common/* provides the usual libraries (also included in other versions of `TreeNET`) to handle IPv4 addresses, network addresses, etc.

* Keep in mind, if you intend to remove or add files to the source code, that you have to edit the subdir.*mk* files in *Release/src/* accordingly. You should also edit *makefile* and sources.*mk* in *Release/* whenever you create a new (sub-)directory in *src/*.
  
* If you want to obtain all the details about the analysis of a tree, it is recommended to redirect the standard output to a file:
  
  ```sh
  sudo ./treenet_architect Some_input_file > Some_output_file.txt
  ```
  
  Note that the file might get quite big for large datasets.

## Changes history

* **December 21, 2016:** first release of *Architect*, as v3.0.

## Disclaimer

`TreeNET` v3 and its different versions were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
