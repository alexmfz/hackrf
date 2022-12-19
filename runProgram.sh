
#!/bin/bash
if  [[ -z "$1" || -z "$2" ]]
then
    echo "Was not possible to execute."
    echo "Frequency range values not selected"
    echo "Example: './runProgram.sh 45 245'"
else
    echo "...Running Compiler..."
    
    cd /home/manolo/hackrf
    ./runCompiler.sh
    
    echo "...Compiler Finished..."
    
    echo "...Running Program..."
    
    cd /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder
    ./hackrf_sweep -f$1:$2
    
    echo "...Program Finished..."
    echo "...Moving fits file into Result Folder"
    
    mv *.fit Result/
    echo "...Opening JavaViewer..."
    
    cd Result
    java -jar RAPPViewer.jar
fi


