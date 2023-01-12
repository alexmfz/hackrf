#!/bin/bash
filename='scheduler.cfg'

time_now=$(date +%H%M%S)

while read schedule_time;
do

	schedule_time_formated=$(date -d "$schedule_time" +"%H%M%S")
	if [ $time_now -gt $schedule_time_formated ]
	then
		echo "Cannot be scheduled on the pass"
	else
		echo "Executing script with param $schedule_time"
		./scheduler_mock -t $schedule_time
	fi
	
done < $filename
