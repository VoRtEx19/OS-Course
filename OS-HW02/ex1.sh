#!/bin/bash
#printing that program started
echo "Script started:"
#reading two values
echo -n "Please enter first value -> "
read a
echo -n "Please enter second value -> "
read b
#print both values
echo "a = $a"
echo "b = $b"

# check if equal, or which is greater
if [ $a -eq $b ]
then
	echo "Both values are equal"
else 
	if [ $a -gt $b ]
	then
		echo "a is greater than b"
	else
		echo "a is less than b"
	fi
fi
echo "The end"
