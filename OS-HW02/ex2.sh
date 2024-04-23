#!/bin/bash
echo "The script is started:"
sum = 0
while (( $sum <= 100 ))
do
	echo "Current sum is $sum"
	echo -n "Enter number -> "
	read a
	sum = $[$sum + $a]
done
echo "Sum is $sum"
echo "The end"
