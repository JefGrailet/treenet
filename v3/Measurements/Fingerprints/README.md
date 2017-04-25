# About Fingerprints/ folder

*By Jean-François Grailet (last edited: April 25, 2017)*

## Overview

This folder contains figures used for the analysis of the list of fingerprints collected for alias resolution on several Autonomous Systems (or ASes) involved in our measurements with `TreeNET`, along other figures related to the alias resolution itself. Figures are classed by dates, and each figure highlights a different aspect of the fingerprints for the 20 ASes which are involved in them.

## ASes

For convenience, the ASes are denoted by an index in the figures. The assigned indexes are given below.

| Index |   AS    | AS name                  | Type    | Max. amount of IPs |
| ----: | :-----: | :----------------------- | :------ | :----------------- |
| 1     | AS109   | Cisco Systems            | Stub    | 1,600,512          |
| 2     | AS10010 | TOKAI Communications     | Transit | 1,860,096          |
| 3     | AS224   | UNINETT                  | Stub    | 1,115,392          |
| 4     | AS2764  | AAPT Limited             | Transit | 1,074,688          |
| 5     | AS5400  | British Telecom          | Transit | 1,385,472          |
| 6     | AS5511  | Orange S.A.              | Transit | 922,880            |
| 7     | AS6453  | TATA Communications      | Tier-1  | 966,144            |
| 8     | AS703   | Verizon Business         | Transit | 873,728            |
| 9     | AS8220  | COLT Technology          | Transit | 1,372,160          |
| 10    | AS8928  | Interoute Communications | Transit | 841,728            |
| 11    | AS12956 | Telefonica International | Tier-1  | 215,040            |
| 12    | AS13789 | Internap Network         | Transit | 106,240            |
| 13    | AS14    | University of Columbia   | Stub    | 339,968            |
| 14    | AS22652 | Fibrenoire, Inc.         | Transit | 76,544             |
| 15    | AS30781 | Jaguar Network           | Transit | 45,824             |
| 16    | AS37    | University of Maryland   | Stub    | 140,544            |
| 17    | AS4711  | INET Inc.                | Stub    | 34,816             |
| 18    | AS50673 | Serverius Holding        | Transit | 65,280             |
| 19    | AS52    | University of California | Stub    | 328,960            |
| 20    | AS802   | University of York       | Stub    | 75,264             |

## Figures

For each date, we provide 8 different figures.

* **AliasMethods_\[year\]_\[date\].pdf:** shows the proportion of aliases which were obtained through a particular method. This figure helps to figure out the relevancy of current state-of-the-art alias resolution techniques. It should be noted that the "Group" bars correspond to cases where no classical method could be used, which forced `TreeNET` to exclusively rely on the space search reduction and similar fingerprint grouping, except when the DNS names (when available) are too different. The "DNS" bars are similar, except that in this case, the fingerprints were almost empty (e.g. <*,*,*,Yes,*>).

* **AliasProportions_\[year\]_\[date\].pdf:** shows the proportion of fingerprinted IPs which were aliased vs. the proportion of IPs which were not aliased to any other IP at all. The goal of this figure is to show that we successfully obtain a large amount of aliases in most cases, despite a few pathological cases where this proportion is rather low. This can be explained by the supposed lack of precision of the subnet inference or the classical issues of `traceroute` which affect the neighborhood inference (which amounts to space search reduction, in this context) of `TreeNET`.

* **Correlation_\[year\]_\[date\].pdf:** shows stacked bar charts of 3 different groups of fingerprints: fingerprints with an initial TTL value of 64 and a *healthy* IP-ID counter class, fingerprints with initial TTL 255 and the *echo* class and the others. The goal of this figure is to show that 64 and *healthy* indeed correlates with each other, by comparing the likeliness of this figure with IPIDClass_ and ITTL_ figures.

* **IPIDClass_\[year\]_\[date\].pdf:** shows stacked bar charts to show the proportion each IP-ID counter "class" among the fingerprints. There are 3 different classes:
  -*echo*: the fingerprinted IP just echoes the IP-ID found in the probe packet in its reply, 
  -*healthy*: the fingerprinted IP provided a different IP-ID than the one which was in the probe packet, and the sequence of IDs collected for that IP could be used to evaluate a speed at which IDs increase, 
  -*random*: same as *healthy*, except a speed could not be obtained because the sequence is not sound.
  There is also a 4th class represented in the graph: "*Others*", denoting the fingerprints for which no class could be derived.

* **ITTL_\[year\]_\[date\].pdf:** shows stacked bar charts, just like above, but this time illustrates the inferred initial TTL of the echo reply from the probed IP, if responsive. There are 4 typical values: 32, 64, 128 and 255.

* **(January 2017 only) ProbeAmount_\[year\]_\[date\].pdf:** shows **non-**bar charts on a logarithmic scale to demonstrate the benefits of `TreeNET` regarding probe amount, by comparing this amount with predicted amount of probes used by `MIDAR`, `RadarGun` and `Ally` (3 state-of-the-art alias resolution tools).

* **TSRequest_\[year\]_\[date\].pdf:** shows the proportion of fingerprinted IPs which replied (or not) to ICMP timestamp requests.

* **SSR_\[year\]_\[date\].pdf:** shows the proportion of fingerprinted IPs with respect to the total of responsive IPs detected by `TreeNET`. A black curve also shows the size of the largest list of fingerprints (~= largest neighborhood) with respect to the total amount of responsive IPs. The purpose of this figure is to show the benefits of the space search reduction achieved by `TreeNET`, which is a consequence of the subnet inference and network tree structure.

## Scripts/ sub-folder

All figures were obtained through Python scripts. Interested readers can view these scripts in the *Scripts [...]/* sub-folders. There are two sub-folders: one for scripts which were used in 2016, and one with those used in 2017 (starting from January). The scripts from the 2017 folder are more convenient to use though the main body of each is the same as before. Note that the scripts should be edited to take account of user's filesystem, and that they all need some input file to have a list of the concerned ASes. Examples of such files are provided.

Also note that the output files produced by the scripts from 2016 are suffixed with a last part after the date (e.g., "_AllASes" or "_ASesList" if you use the provided example files). This references the input file that listed the ASes which were considered for the figures. This suffix is no longer added by the scripts for 2017 for the sake of convenience.
