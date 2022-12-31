#!/bin/bash
originalPath=$(pwd)

if  [[ -z "$1" || -z "$2" || -z "$3" ]]
then
    echo "Was not possible to execute."
    echo "Frequency range values not selected"
    echo "Example: './runProgram.sh 45 245' [0 (C generation) or 1 (python generation)] "
else
    echo "...Running Compiler..."
    
    ./runCompiler.sh
    
    echo "...Compiler Finished..."
    
    echo "...Running Program..."

    cd $originalPath/host/hackrf-tools/src/FitsFolder/

    if [ "$3" -eq 1 ]
    then
      echo "...Fits file will be generate with C script..."
      ./hackrf_sweep -f$1:$2 -p$3
      echo "...Moving fits and logs into Result folder"
      mv *.fit Result/LastResult
      mv *_logs.txt Result/LastResult
      
    else
      echo "...Fits file will be generate with Python script..."
      ./hackrf_sweep -f$1:$2 -p$3
      mv samples.txt pythonScripts/
      mv times.txt pythonScripts/
      mv frequencies.txt pythonScripts/
      mv header_times.txt pythonScripts/
      mv *_logs.txt Result/LastResult

      cd pythonScripts
      python3 generationFits.py
      rm samples.txt
      rm times.txt
      rm frequencies.txt
      rm header_times.txt
      
      echo "...Moving fits and logs into Result folder"

      cd ..

      mv pythonScripts/*.fit Result/LastResult
      mv pythonScripts/*_logs.txt Result/LastResult
    fi

    echo "...Program Finished..."
    echo "...Opening JavaViewer..."
    cd Result/LastResult/
    java -jar RAPPViewer.jar
fi