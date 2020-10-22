#!/bin/bash

if [ $# -lt 1 ]; then
	echo "#(nodes) is required.";
	exit 1;
fi

for i in `seq 1 $1`; do
	echo "NODE b$i dumps:";
	# cat b$i-output.txt | grep -v "DEBUG";
	cat b$i-output.txt | grep "INFO";
	echo "";
done
