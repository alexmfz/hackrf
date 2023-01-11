#!/bin/bash
originalPath=$(pwd)

if  [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" || -z "$5" ]]
then
    echo "Was not possible to execute."
    echo "Example: './runProgram.sh fmin fmax' generationMode stationName Focus"
    echo "Example: './runProgram.sh 45 245' 0 SPAIN-PERALEJOS 63"
    echo "generationMode == 1  -> C generation or generationMode == 0 -> Python generation"
else   

    echo "...Moving previous Results into PreviousResults folder..."
    cd $originalPath/Result/
    mv LastResult/*.fit PreviousResults
    mv LastResult/*_logs.txt PreviousResults


    cd $originalPath
    rm samples.txt
    rm header_times.txt
    rm times.txt
    rm frequencies.txt

    echo "...Running Program..."

    if [ "$3" -eq 1 ]
    then
      echo "...Fits file will be generate with C script..."
      ./hackrf_sweep_rpi3 -f$1:$2 -c$3 -s$4 -z$5
      echo "...Moving fits and logs into Result folder"
      mv *.fit Result/LastResult
      mv *_logs.txt Result/LastResult
      
    else
      echo "...Fits file will be generate with Python script..."
      ./hackrf_sweep_rpi3 -f$1:$2 -c$3 -s$4 -z$5
      mv *_logs.txt Result/LastResult
      python3 generationFits.py $4 $5
      rm samples.txt
      rm times.txt
      rm frequencies.txt
      rm header_times.txt
      
      echo "...Moving fits and logs into Result folder"

      mv *.fit Result/LastResult
      mv *_logs.txt Result/LastResult
    fi

    echo "...Program Finished..."
    echo "...Opening JavaViewer..."
    cd Result/LastResult/
    java -jar RAPPViewer.jar
fi
