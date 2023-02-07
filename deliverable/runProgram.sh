#!/bin/bash
echo "...Reattaching HackRF One..."
./hackrf_spiflash -R  # Restart script
sleep 1

originalPath=$(pwd) # Base path

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
rm original.tmp
rm scheduler.tmp

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

control_external_generation=$(head -n 14 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^control_external_generation=].*')

# Write 0 always at beginning to avoid generation at beginning
sed -i 's\control_external_generation=1\control_external_generation=0\' $parameter_file 

#Checks parameters of execution an run it if everything is ok
if  [[ -z "$freq_min" || -z "$freq_max" || -z "$gen_mode" ||
       -z "$station_name" || -z "$focus_code" || -z "$gain" ||
       -z $longitude || -z $longitude_code ||
       -z $latitude || -z $latitude_code ||
       -z $altitude ||
       -z $object || -z $content ||
       -z $control_external_generation ]]
then
    echo "Was not possible to execute."
    echo "Check config.cfg"
    echo "...Exiting..."
    exit 0
    
else   
    # Move last results into previous results folder
    echo "...Moving previous Results into PreviousResults folder..."
    cd $originalPath/Result
    mv LastResult/*.fit PreviousResults
    mv LastResult/*_logs.txt PreviousResults

    echo "...Running Program..."
    
    cd $originalPath

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
        if [ "$gen_mode" -eq 1 ]
        then
          echo "...Fits file will be generate with C script..."
          ./hackrf_sweep $execution_argument
                      
          mv *.fit Result/LastResult
          mv *_logs.txt Result/LastResult
       
        else
          echo "...Fits file will be generate with Python script..."
            ./hackrf_sweep $execution_argument
            mv *_logs.txt Result/LastResult
            # Enable control flag to execute python script
            sed -i 's\control_external_generation=0\control_external_generation=1\' $parameter_file 

        fi
      fi
    done < $scheduler_file

    echo "...Program Finished..."
    #echo "...Opening JavaViewer..."
    #cd Result/LastResult/
    #java -jar RAPPViewer.jar
  
fi
