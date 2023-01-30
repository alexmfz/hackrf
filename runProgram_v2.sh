#!/bin/bash
echo "...Reattaching HackRF One..."
./hackrf_spiflash -R  # Restart script
sleep 1

originalPath=$(pwd) # Base path
cd $originalPath/host/hackrf-tools/src/FitsFolder

parameter_file='config.cfg'
scheduler_file='scheduler.cfg' # File name with schedule times

content_scheduling=$(cat $scheduler_file) # Content of file name
content_parameter=$(cat $parameter_file) # Content of file name with times

check_comment=$(cat $scheduler_file | grep "END SCHEDULING") # Checks if exists comment in filename
check_format_1="^[0-1][0-9]:[0-5][0-9]:[0-5][0-9]" # Checks times from 00:00:00 to 19:59:59
check_format_2="^[2][0-3]:[0-5][0-9]:[0-5][0-9]" # Checks times from 20:00:00 to 23:59:59

time_now=$(date +%H%M%S) # Time at this moment

# Checks file content
cp $scheduler_file original.tmp
head -n -1 $scheduler_file > scheduler.tmp
cp scheduler.tmp $scheduler_file

if [[ ! -z "$content_scheduling" && -s $scheduler_file ]]
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
		check_time_1=$(echo $schedule_time | grep $check_format_1)
		check_time_2=$(echo $schedule_time | grep $check_format_2)
		if [[ -z $check_time_1 && -z $check_time_2 ]]
		then
			echo "Time not well formated"
			echo "...Exiting..."
			exit 0
		fi

		
	done < $scheduler_file

else
	echo "file empty"
	echo "Example (comment must be included):
	20:00:00
	20:15:00
	########### END SCHEDULING ###########"
	echo "...Exiting..."
	exit 0
fi

cp original.tmp $scheduler_file


# Checks file content (parameters.cfg)
if [[ -z "$content_parameter" && -s $parameter_file ]]
then
	echo "file config.cfg empty"
	echo "...Exiting..."
	exit 0
fi

# Take parameters to set as input and check that are not empty
freq_min=$(head -n 1 $parameter_file | grep -o '^[^#]*' | grep -o '[^frq_min=]*')
freq_max=$(head -n 2 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^frq_max=].*')

gen_mode=$(head -n 3 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gen_mode=].*')

station_name=$(head -n 4 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^station_name=].*')
focus_code=$(head -n 5 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^focus_code=].*')

gain=$(head -n 6 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gain=].*')

longitude=$(head -n 7 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude=].*')
longitude_code=$(head -n 8 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude_code=].*')

latitude=$(head -n 9 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude=]*')
latitude_code=$(head -n 10 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude_code=].*')

altitude=$(head -n 11 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^altitude=].*')

object=$(head -n 12 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^object=].*')
content=$(head -n 13 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^content=].*')

echo "Freq min: $freq_min"
echo "Freq max: $freq_max"
echo "Generation Mode: $gen_mode"
echo "Station Name: $station_name"
echo "Focus Code: $focus_code"
echo "Gain: $gain"
echo "Longitude: $longitude"
echo "Longitude Code: $longitude_code"
echo "Latitude: $latitude"
echo "Latitude Code: $latitude_code"
echo "Altitude: $altitude"
echo "Object: $object"
echo "Content: $content"

#Checks parameters of execution an run it if everything is ok
if  [[ -z "$freq_min" || -z "$freq_max" || -z "$gen_mode" ||
       -z "$station_name" || -z "$focus_code" || -z "$gain" ||
       -z $longitude || -z $longitude_code ||
       -z $latitude || -z $latitude_code ||
       -z $altitude ||
       -z $object || -z $content ]]
then
    echo "Was not possible to execute."
    echo "Check config.cfg"
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
      execution_argument="-f$(echo $freq_min:$freq_max | tr -d '[:space:]') -c$gen_mode -s$station_name -z$focus_code -t$schedule_time -g$gain -L$longitude -k$longitude_code -m$latitude -M$latitude_code -A$altitude -o$object -O$content"

      if [ $time_now -gt $schedule_time_formated ]
	    then
		    echo "Cannot be scheduled on the pass"
	    else
		    echo "Executing script with param $schedule_time"
          
        # Checks if the generation of fits file is with C or python
        cd $originalPath/host/hackrf-tools/src/FitsFolder/
        if [ "$gen_mode" -eq 1 ]
        then
        echo "...Fits file will be generate with C script..."
          ./hackrf_sweep $execution_argument
                         
          mv *.fit Result/LastResult
          mv *_logs.txt Result/LastResult
          exit 0

        else
          echo "...Fits file will be generate with Python script..."
            ./hackrf_sweep $execution_argument
          mv samples.txt pythonScripts/
          mv times.txt pythonScripts/
          mv frequencies.txt pythonScripts/
          mv header_times.txt pythonScripts/
          mv *_logs.txt Result/LastResult

        fi
      fi
    done < $scheduler_file

    echo "...Program Finished..."
    #echo "...Opening JavaViewer..."
    #cd Result/LastResult/
    #java -jar RAPPViewer.jar
  
fi
