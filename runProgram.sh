
#!/bin/bash
if  [[ -z "$1" || -z "$2"]]
then
    echo "Was not possible to execute."
    echo "Frequency range values not selected"
    echo "Example: './runProgram.sh 45 245' [0 (C generation) or 1 (python generation)] "
else
    echo "...Running Compiler..."
    
    cd /home/manolo/hackrf
    ./runCompiler.sh
    
    echo "...Compiler Finished..."
    
    echo "...Running Program..."

    cd /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder

    if [ "$3" -eq 0 ]
    then
      echo "...Fits file will be generate with C script..."
      ./hackrf_sweep -f$1:$2 -p$3
      echo "...Moving fits into Result folder"
      mv *.fit Result/
    else
      echo "...Fits file will be generate with Python script..."
      ./hackrf_sweep -f$1:$2 -p$3
      mv samples.txt /pythonScripts
      mv times.txt /pythonScripts
      mv frequencies.txt /pythonScripts
      cd pythonScripts
      python3 generationFits.py
      echo "...Moving fits into Result folder"
      mv *.fit /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder/Result
    fi

    echo "...Program Finished..."
    echo "...Opening JavaViewer..."
    cd Result
    java -jar RAPPViewer.jar
fi


