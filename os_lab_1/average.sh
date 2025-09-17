#!/bin/bash

if [ $# -eq 0 ]; then 
	echo "Error! Arguments are not passed to the script!"
	exit 1
fi

count=$#
sum=0

for num in "$@"; do
	sum=$((sum + num))
done

average=$((sum / count))

echo "Amount of the numbers passed: $count"
echo "Sum of the numbers passed: $sum"
echo "Average of the numbers passed: $average"
