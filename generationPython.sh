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
logger=1 # Variable to show waiting message

# Checks file content (parameters.cfg)
if [[ -z "$content_parameter" && -s $parameter_file ]]
then
	echo "file parameters.cfg empty"
	echo "...Exiting..."
	exit 0
fi

# Take parameters to set as input and check that are not empty
freq_min=$(head -n 1 $parameter_file | grep -o '^[^#]*' | grep -o '[^frq_min=]*' | tr -d '[:space:]')
freq_max=$(head -n 2 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^frq_max=].*' | tr -d '[:space:]')
gen_mode=$(head -n 3 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gen_mode=].*' | tr -d '[:space:]')
station_name=$(head -n 4 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^station_name=].*' | tr -d '[:space:]')
focus_code=$(head -n 5 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^focus_code=].*' | tr -d '[:space:]')
gain=$(head -n 6 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^gain=].*' | tr -d '[:space:]')
longitude=$(head -n 7 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude=].*' | tr -d '[:space:]')
longitude_code=$(head -n 8 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^longitude_code=].*' | tr -d '[:space:]')
latitude=$(head -n 9 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude=]*' | tr -d '[:space:]')
latitude_code=$(head -n 10 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^latitude_code=].*' | tr -d '[:space:]')
altitude=$(head -n 11 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^altitude=].*' | tr -d '[:space:]')
object=$(head -n 12 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^object=].*' | tr -d '[:space:]')
content=$(head -n 13 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^content=].*' | tr -d '[:space:]')
control_external_generation=$(head -n 14 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^control_external_generation=].*' | tr -d '[:space:]')

if  [[ -z "$freq_min" || $freq_min -eq $empty ||
       -z "$freq_max" || $freq_max -eq $empty ||
       -z "$gen_mode" || $gen_mode -eq $empty ||
       -z "$station_name" || "$station_name" == "$empty" ||
       -z "$focus_code" || $focus_code == $empty ||
       -z "$gain" || $gain == $empty ||
       -z "$longitude" || $longitude == $empty ||       
       -z "$longitude_code" || $longitude_code == $empty ||
       -z "$latitude" || $latitude == $empty ||
       -z "$latitude_code" || $latitude_code == $empty ||
       -z "$altitude" || $altitude == $empty || 
       -z "$object" || $object == $empty ||
       -z "$content" || $content == $empty || 
       -z "$control_external_generation" || $control_external_generation == $empty 
      ]]
then
    echo "File parameters are empty"
    echo "...Exiting..."
    exit 0
else
    # Check gen mode and if gen_mode == 0, then scheduling.cfg will be read and sw will be executed
    if [ $gen_mode -eq 0 ]
    then        
        while [ $control_external_generation -ne 2 ]
        do
            control_external_generation=$(head -n 14 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^control_external_generation=].*')
            if [ $control_external_generation -eq 1 ]
            then
                # execute generation with python
                cd pythonScripts/
                python3 generationFits.py $station_name $focus_code $latitude $latitude_code $longitude $longitude_code $altitude $object $content
                rm samples.txt
                rm times.txt
                rm frequencies.txt
                rm header_times.txt

                cd ..       
            
                mv pythonScripts/*.fit Result/LastResult
                mv pythonScripts/*_logs.txt Result/LastResult
                                
                # Disable control flag to not execute python script again
                sed -i 's\control_external_generation=1\control_external_generation=0\' $parameter_file
                logger=1
            #else

             #   if [[ $logger -eq 1 && $control_external_generation -ne 2 ]]
              #  then
               #     echo "...Waiting until sweeping is done..."
                #    logger=0                
                #fi

            fi 
        done

    else 
        echo "C generation. You dont need to execute this."
        echo "If you want to execute python generation change gen_mode to 0 at config.cfg."
        echo "...Exiting..."
        exit 0
    
    fi

fi

exit 0