#!/bin/bash
originalPath=$(pwd) # Base path
cd $originalPath/host/hackrf-tools/src/FitsFolder

parameter_file='config.cfg' # File with parameters of the program to be executed
scheduler_file='scheduler.cfg' # File with scheduling times

content_scheduling=$(cat $scheduler_file) # Content of file name with times
content_parameter=$(cat $parameter_file) # Content of file name with times

check_comment=$(cat $scheduler_file | grep "END SCHEDULING") # Checks if exists comment in filename
check_format_1="^[0-1][0-9]:[0-5][0-9]:[0-5][0-9]" # Checks times from 00:00:00 to 19:59:59
check_format_2="^[2][0-3]:[0-5][0-9]:[0-5][0-9]" # Checks times from 20:00:00 to 23:59:59

time_now=$(date +%H%M%S) # Time at this moment

# Checks file content (scheduler.cfg)
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
	echo "file scheduler.cfg empty"
	echo "Example (comment must be included):
	20:00:00
	20:15:00
	########### END SCHEDULING ###########"
	echo "...Exiting..."
	exit 0
fi

# Checks file content (parameters.cfg)
if [[ -z "$content_parameter" && -s $parameter_file ]]
then
	echo "file parameters.cfg empty"
	echo "...Exiting..."
	exit 0
fi

# Take parameters to set as input and check that are not empty
freq_min=$(head -n 1 $parameter_file | grep -o '^[^#]*' | grep -o '[^frq_min=]*')
freq_max=$(head -n 2 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^frq_max=]*')
gen_mode=$(head -n 3 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gen_mode=]*')
station_name=$(head -n 4 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^station_name=].*')
focus_code=$(head -n 5 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^focus_code=]*')
gain=$(head -n 6 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gain=]*')
longitude=$(head -n 7 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude=]*')
longitude_code=$(head -n 8 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude_code=].*')
latitude=$(head -n 9 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude=]*')
latitude_code=$(head -n 10 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude_code=].*')
altitude=$(head -n 11 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^altitude=]*')
object=$(head -n 12 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^object=].*')
content=$(head -n 13 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^content=].*')

# Check the content of the variables
if [[ -z $freq_min || -z $freq_max || -z $gain ||
      -z $gen_mode   || -z $station_name || -z $focus_code || 
      -z $longitude  || -z $longitude_code ||
      -z $latitude   || -z $latitude_code  || -z $altitude ||
      -z $object   || -z $content ]]
then
    echo "File parameters are empty"
    echo "...Exiting..."
    exit 0
else
    # Check gen mode and if gen_mode == 0, then scheduling.cfg will be read and sw will be executed
    if [ $gen_mode -eq 0 ]
    then        
        while read schedule_time
        do
            # Add time to have more margin
            schedule_time_future=$(date -d "$schedule_time  + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds
                                                            + 1 seconds" +"%H%M%S")
            while [ $time_now -lt $schedule_time_future ]
            do
                time_now=$(date +%H%M%S) 
            done

            # execute generation with python
            cd pythonScripts/
            python3 generationFits.py $station_name $focus_code $latitude $latitude_code $longitude $longitude_code $altitude $object $content
            rm samples.txt
            rm times.txt
            rm frequencies.txt
            rm header_times.txt

            cd ..       
            # sed -i 's\control_external_generation=1\control_external_generation=0\' $parameter_file # TODO: Checks
            
            mv pythonScripts/*.fit Result/LastResult
            mv pythonScripts/*_logs.txt Result/LastResult

        done < $scheduler_file
    else 
        echo "C generation. You dont need to execute this"
        echo "...Exiting..."
        exit 0
    
    fi

fi