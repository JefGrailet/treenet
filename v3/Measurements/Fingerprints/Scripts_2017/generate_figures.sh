#!/bin/bash

year=$1
date=$2
ASFile=$3

python AliasMethods.py $year $date $ASFile
python AliasProportions.py $year $date $ASFile
python FingerprintsAnalysisCorrelation.py $year $date $ASFile
python FingerprintsAnalysisIPIDClass.py $year $date $ASFile
python FingerprintsAnalysisITTL.py $year $date $ASFile
python FingerprintsAnalysisSSR.py $year $date $ASFile
python FingerprintsAnalysisTSRequest.py $year $date $ASFile

mkdir -p ./Figures/$year/$date
mv *.pdf ./Figures/$year/$date
