#!/bin/bash

rm numbers.txt 2> /dev/null

od -t u2 -An -N300 /dev/random |
	tr -s ' ' '\n' | 
	grep -v '^$' > numbers.txt

head numbers.txt
wc -l numbers.txt
