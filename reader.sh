#!/bin/bash
filename='scheduler.cfg'

time_now=$(date +%H%M%S)

while read schedule_time;
do
	# read each line
	#echo "time scheduled "$line
	#echo "time now:" $time_now
	#echo "time scheduled:" $schedule_time
			
	if [ $time_now -gt $schedule_time ]
	then
		echo "Cannot be scheduled on the pass"
	else
		echo "Executing script"
	fi
	
done < $filename
