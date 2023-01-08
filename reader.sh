#!/bin/bash
filename='scheduler.cfg'

n=1
while read line;
do
	# read each line
	echo "Line No. $n: $line"
	n=$((n+1))
done < $filename
