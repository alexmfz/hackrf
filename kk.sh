#!/bin/bash
echo "...Reattaching HackRF One..."
./hackrf_spiflash -R  # Restart script
sleep 1
originalPath=$(pwd) # Base path

filename='scheduler.cfg' # File name with schedule times
content=$(cat $filename) # Content of file name
check_comment=$(cat $filename | grep "END SCHEDULING") # Checks if exists comment in filename
check_format_1="^[0-1][0-9]:[0-5][0-9]:[0-5][0-9]" # Checks times from 00:00:00 to 19:59:59
check_format_2="^[2][0-3]:[0-5][0-9]:[0-5][0-9]" # Checks times from 20:00:00 to 23:59:59
time_now=$(date +%H%M%S) # Time at this moment

# Checks file content
cd $originalPath/host/hackrf-tools/src/FitsFolder
head -n -1 $filename > scheduler.tmp
cp scheduler.tmp $filename
if [[ ! -z "$content" && -s $filename ]]
then
	if [ -z "$check_comment" ]
	then
		echo "File not empty, but not well formated.
		Must include this exactly comment:
		########### END SCHEDULING ###########"
		echo "...Exiting..."
		exit 0
	
	fi

	while read schedule_time
	do
		echo $schedule_time
		check_time_1=$(echo $schedule_time | grep $check_format_1)
		check_time_2=$(echo $schedule_time | grep $check_format_2)
		echo $check_time_1
		echo $check_time_2

		if [[ -z $check_time_1 && -z $check_time_2 ]]
		then
			echo "Time not well formated"
			echo "...Exiting..."
			exit 0
		fi

		
	done < $filename

else
	echo "file empty"
	echo "Example (comment must be included):
	20:00:00
	20:15:00
	########### END SCHEDULING ###########"
	echo "...Exiting..."
	exit 0
fi


#Checks parameters of execution an run it if everything is ok
if  [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" || -z "$5" ]]
then
    echo "Was not possible to execute."
    echo "Example: './runProgram.sh fmin fmax' generationMode stationName Focus"
    echo "Example: './runProgram.sh 45 245' 0 SPAIN-PERALEJOS 63"
    echo "generationMode == 1  -> C generation or generationMode == 0 -> Python generation"
    echo "...Exiting..."
    exit 0
    
else   

    # Move last results into previous results folder
    echo "...Moving previous Results into PreviousResults folder..."
    cd $originalPath/host/hackrf-tools/src/FitsFolder/Result/
    mv LastResult/*.fit TestResults
    mv LastResult/*_logs.txt TestResults

    echo "...Running Program..."
    
    cd $originalPath/host/hackrf-tools/src/FitsFolder/

    while read schedule_time;
    do
      schedule_time_formated=$(date -d "$schedule_time" +"%H%M%S")

      if [ $time_now -gt $schedule_time_formated ]
	    then
		    echo "Cannot be scheduled on the pass"
	    else
		    echo "Executing script with param $schedule_time"
          
        # Checks if the generation of fits file is with C or python
        cd $originalPath/host/hackrf-tools/src/FitsFolder/
        if [ "$3" -eq 1 ]
        then
        #echo "...Fits file will be generate with C script..."
          ./hackrf_sweep -f$1:$2 -c$3 -s$4 -z$5 -t$schedule_time
          mv *.fit Result/LastResult
          mv *_logs.txt Result/LastResult

        else
         # echo "...Fits file will be generate with Python script..."
          ./hackrf_sweep -f$1:$2 -c$3 -s$4 -z$5 -t$schedule_time
          mv samples.txt pythonScripts/
          mv times.txt pythonScripts/
          mv frequencies.txt pythonScripts/
          mv header_times.txt pythonScripts/

          cd pythonScripts/
          python3 generationFits.py $4 $5
          rm samples.txt
          rm times.txt
          rm frequencies.txt
          rm header_times.txt

          cd ..
      
        fi
      fi
    done < $filename

    echo "...Moving fits and logs into Result folder"
    mv *_logs.txt Result/LastResult
    mv pythonScripts/*.fit Result/LastResult
    mv pythonScripts/*_logs.txt Result/LastResult
    # Execute FITS viewer
    echo "...Program Finished..."
    echo "...Opening JavaViewer..."
    cd Result/LastResult/
    java -jar RAPPViewer.jar
  
fi
