#!/bin/bash
echo "INFO: Reattaching HackRF One"
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
head -n -2 $scheduler_file > scheduler.tmp
cp scheduler.tmp $scheduler_file

if [[ ! -z "$content_scheduling" && -s $scheduler_file ]]
then
	if [ -z "$check_comment" ]
	then
		echo "ERROR: File not empty, but not well formated.
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
			echo "ERROR: Time not well formated"
			echo "...Exiting..."
			exit 0
		fi

		
	done < $scheduler_file

else
	echo "ERROR: File empty"
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
	echo "ERROR: File config.cfg empty"
	echo "...Exiting..."
	exit 0
fi

cp original.tmp $scheduler_file

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

# Periodity part 
period_time=$(head -n 15 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^period_time=].*')

check_time_3=$(echo $period_time | grep $check_format_1)		
check_time_4=$(echo $period_time | grep $check_format_2)
if [[ -z $check_time_3 && -z $check_time_4 ]]
    then
      echo "ERROR: Time periodity not well formated. Check it at config.cfg"
      echo "...Exiting..."
      exit 0
fi
time_check_repetition=$(date -d "$period_time" +"%H%M%S"  | tr -d '[:space:]' | sed 's/^0*//') # Period time formated

enable_repetition=0 # Control flag periodicity
control_log=1 # Log control flag
first_execution=0 # Control flag first execution

#Checks parameters of execution an run it if everything is ok
if  [[ -z "$freq_min" || -z "$freq_max" || -z "$gen_mode" ||
       -z "$station_name" || -z "$focus_code" || -z "$gain" ||
       -z $longitude || -z $longitude_code ||
       -z $latitude || -z $latitude_code ||
       -z $altitude ||
       -z $object || -z $content ||
       -z $control_external_generation ||
       -z $time_check_repetition ]]
then
    echo "ERROR: Was not possible to execute."
    echo "Check config.cfg"
    echo "...Exiting..."
    exit 0
    
else
    # Periodically execution
    while [ 1 ]
    do
        time_now=$(date +%H%M%S | sed 's/^0*//') # Update time

        # Checks Control log to show log at 2nd or consecutive executions 
        if [[ $control_log -eq 1 && $first_execution -eq 1 ]]
        then          
          period_time=$(head -n 15 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^period_time=].*')
          check_time_3=$(echo $period_time | grep $check_format_1)		
          check_time_4=$(echo $period_time | grep $check_format_2)
          if [[ -z $check_time_3 && -z $check_time_4 ]]
              then
                echo "ERROR: Time periodity not well formated. Check it at config.cfg"
                echo "...Exiting..."
                exit 0
          fi
          time_check_repetition=$(date -d "$period_time" +"%H%M%S"  | tr -d '[:space:]' | sed 's/^0*//') # Period time formated

          freq_min=$(head -n 1 $parameter_file | grep -o '^[^#]*' | grep -o '[^frq_min=]*')
          freq_max=$(head -n 2 $parameter_file | tail -n 1 | grep -o '^[^#]*' | grep -o '[^frq_max=].*')
          
          echo "INFO: Program will be executed again at $period_time"
          ./hackrf_spiflash -R # restart HACKRF ONE  
          sleep 1
            
          control_log=0
        fi

        # Execute functionality at first execution or when times are equals in the following ones
        if [[ $first_execution -eq 0 || ($time_now -eq $time_check_repetition && $enable_repetition -eq 0) ]]
	      then
		      enable_repetition=1
          first_execution=1
          
          cp $scheduler_file original.tmp
          head -n -2 $scheduler_file > scheduler.tmp
          cp scheduler.tmp $scheduler_file

          # Write 0 always at beginning to avoid generation at beginning
          sed -i 's\control_external_generation=1\control_external_generation=0\' $parameter_file
          sed -i 's\control_external_generation=2\control_external_generation=0\' $parameter_file
  
          if [ "$gen_mode" -eq 0 ]
          then
            cd $originalPath
            echo "INFO: Executing fits generator"
            ./generationPython.sh &
          fi
          
          if [ $enable_repetition -eq 1 ]
          then

            echo "INFO: Running Program"

            # HERE
            while read schedule_time;
              do
                schedule_time_formated=$(date -d "$schedule_time" +"%H%M%S" | sed 's/^0*//')
                execution_argument="-f$(echo $freq_min:$freq_max | tr -d '[:space:]') -c$gen_mode -s$station_name -z$focus_code -t$schedule_time -g$gain -L$longitude -k$longitude_code -m$latitude -M$latitude_code -A$altitude -o$object -O$content"
              
                if [ $time_now -gt $schedule_time_formated ]
                then
                  echo "ERROR. Cannot be scheduled on the pass"
                  exit 0
                else
                  echo "INFO: Executing script at $schedule_time"
                      
                  # Checks if the generation of fits file is with C or python
                  if [ "$gen_mode" -eq 1 ]
                  then
                    echo "INFO: Fits file will be generate with C script"
                    ./hackrf_sweep $execution_argument
                                 
                    mv *.fit Result/LastResult
                    mv *_logs.txt Result/LastResult
                     
                    echo -e "INFO: Fits file generated\n"
                  
                  else
                    echo "INFO: Fits file will be generate with Python script"
                    ./hackrf_sweep $execution_argument
                    mv *_logs.txt Result/LastResult
                
                    # Enable control flag to execute python script
                    sed -i 's\control_external_generation=0\control_external_generation=1\' $parameter_file 
                    echo -e "INFO: Fits file generated\n"

                  fi

                fi

              done < $scheduler_file
              cp original.tmp $scheduler_file

              # Kill generationPython.sh
              sleep 4
              sed -i 's\control_external_generation=1\control_external_generation=2\' $parameter_file 
              sed -i 's\control_external_generation=0\control_external_generation=2\' $parameter_file 
                
              echo -e "INFO: Program Finished For Today. Waiting until next execution\n"
              #echo "...Opening JavaViewer..."
              #cd Result/LastResult/
              #java -jar RAPPViewer.jar
              
          fi

          #end
          enable_repetition=0 # disable repetition until time_now == time_check_repetition
          control_log=1 # Enable log of future executions
        fi	

    done  
fi
