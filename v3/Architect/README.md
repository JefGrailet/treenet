# About TreeNET v3.3 "Architect" (treenet_architect)

*By Jean-François Grailet, August 29, 2017*

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

`TreeNET` v3.3 "*Architect*" will describe in details its options, flags and how you can use it by running the line:

```sh
./treenet_architect -h
```

## Remarks

* Unlike *Arborist* and *Forester*, *Architect* is a completely passive tool that will not conduct any measurement of any kind. It is solely dedicated to the conversion of datasets into graphs.

* Most of the actual code of *Architect* is found in *src/treenet/*. *src/common/* provides the usual libraries (also included in other versions of `TreeNET`) to handle IPv4 addresses, network addresses, etc.

* Keep in mind, if you intend to remove or add files to the source code, that you have to edit the subdir.*mk* files in *Release/src/* accordingly. You should also edit *makefile* and sources.*mk* in *Release/* whenever you create a new (sub-)directory in *src/*.
  
* If you want to obtain all the details about the analysis of a tree, it is recommended to redirect the standard output to a file:
  
  ```sh
  sudo ./treenet_architect Some_input_file > Some_output_file.txt
  ```
  
  Note that the file might get quite big for large datasets.

* *Architect* aims at producing graphs that consist of a single connected component. It has been thoroughly tested and reviewed with many datasets to ensure this is always the case. However, if for some reason a produced graph consists of several connected components, feel free to contact me (see below) to discuss the matter.

## Changes history

* **December 21, 2016:** first release of *Architect*, as v3.0.

* **January 10, 2017:** upgrade of *Architect* to v3.0.1. In addition to fixing a light issue in the alias resolution part and updating a data structure, this new version brings a built-in graph verification feature to check if a generated graph indeed consists of a single connected component. The graph generation algorithms have also been reviewed to fix minor issues which caused the conversion to produce a few small-scale connected components in addition to the main one.

* **April 14, 2017:** update to version v3.2. It essentially aligns *Architect* with the lattest changes (in alias resolution and route treatment) implemented in *Arborist* v3.2 and makes some minor changes to the parsing utilities to comply with them. It also times its different algorithmic steps.

* **August 29, 2017:** upgrade of *Architect*, now in version v3.3. This new version essentially takes account of the changes brought to *Arborist* v3.3 to aggregate IP interfaces in the same manner in the multi-label nodes of the network tree. It also takes account of the new aggregation process for the graph transformations.

## Disclaimer

`TreeNET` v3.3 and its different versions were written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN), using the sources of `ExploreNET` v2.1 as a basis.

`ExploreNET` v2.1 has been elaborated and written by Dr. Mehmet Engin Tozal, currently assistant teacher at the University of Louisiana at Lafayette (USA). The sources of this program can be downloaded at the following address:

http://nsrg.louisiana.edu/project/ntmaps/output/explorenet.html

## Contact

**E-mail address:** Jean-Francois.Grailet@ulg.ac.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `TreeNET` (any version). I am also inclined to answer questions regarding the algorithms used in `TreeNET` and to discuss its application in other research projects.
