#!/bin/bash
gcc scheduler_mock.c -o scheduler_mock

filename='scheduler.cfg'
output=$(cat $filename)
check_comment=$(cat $filename | grep "END SCHEDULING")
check_format_1="^[0-1][0-9]:[0-5][0-9]:[0-5][0-9]"
check_format_2="^[2][0-3]:[0-5][0-9]:[0-5][0-9]"

time_now=$(date +%H%M%S)

if [[ ! -z "$output" && -s $filename ]]
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

while read schedule_time;
do	
	schedule_time_formated=$(date -d "$schedule_time" +"%H%M%S")
	if [ $time_now -gt $schedule_time_formated ]
	then
		echo "Cannot be scheduled on the pass"
	else
		echo "Executing script with param $schedule_time"
		#./scheduler_mock -t $schedule_time
	fi
done < $filename