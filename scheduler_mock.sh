#!/bin/bash

time_now=$(date +%H%M%S)
period_time="07:59:00"
#time_check_repetition=$(date -d "$period_time" +"%H%M%S")
time_check_repetition=$(date -d "$period_time" +"%H%M%S" | sed 's/^0*//')
echo $time_check_repetition
enable_repetition=0
control_log=1


# Do it again and again when time_now is 07:59:00
while [ 1 ]
do 
	time_now=$(date +%H%M%S | sed 's/^0*//') # Update time
	if [ $control_log -eq 1 ]
	then 
		echo "Program will be executed again at $period_time"
		control_log=0
	fi
	
#	if [[ $time_now -eq $time_check_repetition && $enable_repetition -eq 0 ]]
	if [[ $time_now -eq $time_check_repetition && $enable_repetition -eq 0 ]]

	then
		enable_repetition=1	

		## Read all the file 
		if [ $enable_repetition -eq 1 ]
		then
			echo "...executing..."
		fi
	fi
	
done
