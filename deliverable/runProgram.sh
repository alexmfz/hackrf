#!/bin/bash
originalPath=$(pwd)

filename='scheduler.cfg' # file that will contain scheduling times

time_now=$(date +%H%M%S) # Time at this moment. Cannot schedule on the pass

content_filename=$(cat $filename) # The content of the file must not be empty

check_comment=$(cat $filename | grep "########### END SCHEDULING ###########") # This comment should be at .cfg

check_format_1="^[0-1][0-9]:[0-5][0-9]:[0-5][0-9]" # datetime format 1
check_format_2="^[2][0-3]:[0-5][0-9]:[0-5][0-9]" # # datetime format 2


if  [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" || -z "$5" ]]
then
    # At least one parameter is null
    echo "Was not possible to execute."
    echo "Example: './runProgram.sh fmin fmax' generationMode stationName Focus"
    echo "Example: './runProgram.sh 45 245' 0 SPAIN-PERALEJOS 63 "
    echo "generationMode == 1  -> C generation or generationMode == 0 -> Python generation"
    echo "...Exiting..."
    exit 0
else   
    # Moving lastResult into PreviousResults
    echo "...Moving previous Results into PreviousResults folder..."
    cd $originalPath/Result/
    mv LastResult/*.fit PreviousResults
    mv LastResult/*_logs.txt PreviousResults

    # Checks format of schedule file
    cd $originalPath
    echo "...Checking scheduler file format..."
    
    if [[ ! -z $content_filename && -s $filename ]]
    then
      # It must include last line as comment ########### END SCHEDULING ###########
      if [ -z "$check_comment" ]
      then
        echo "File not empty, but not well formated.
              Must include this exactly comment as last line:
              ########### END SCHEDULING ###########"
        echo "...Exiting..."
        exit 0
      
      fi

    else
      echo "File empty"
      echo "Example (comment must be included):
		        20:00:00
		        20:15:00
		        ########### END SCHEDULING ###########" 
      echo "...Exiting..."
      exit 0

      # Check time format
      while read schedule_time
      do
        check_time_1=$(echo $schedule_time | grep $check_format_1)
		    check_time_2=$(echo $schedule_time | grep $check_format_2)
        
        if [[ -z $check_time_1 && -z $check_time_2 ]]
		    then
			    echo "Time not well formated"
			    echo "...Exiting..."
			    exit 0
		    fi
     
      done < $filename
    
    fi

    echo "...Running Program..."
    while read schedule_time;
      do
	      schedule_time_formated=$(date -d "$schedule_time" +"%H%M%S")
        if [ $time_now -gt $schedule_time_formated ]
        then
          echo "Cannot be scheduled on the pass: $schedule_time"
        else
          echo "Executing script with param $schedule_time"

          if [ "$3" -eq 1 ]
          then
            echo "...Fits file will be generate with C script..."
            ./hackrf_sweep -f$1:$2 -c$3 -s$4 -z$5 -t$schedule_time
            echo "...Moving fits and logs into Result folder"
            mv *.fit Result/LastResult
            mv *_logs.txt Result/LastResult
      
          else
            echo "...Fits file will be generate with Python script..."
            ./hackrf_sweep -f$1:$2 -c$3 -s$4 -z$5 -t$schedule_time
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
        
        fi
      
      done < $filename

    echo "...Program Finished..."
    echo "...Opening JavaViewer..."
    cd Result/LastResult/
    java -jar RAPPViewer.jar

fi