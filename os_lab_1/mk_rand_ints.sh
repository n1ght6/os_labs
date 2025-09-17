#!/bin/bash

rm numbers.txt 2> /dev/null

for i in {1..150}; do
	echo $((RANDOM % 1000))
done > numbers.txt

head numbers.txt
