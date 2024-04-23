#!/bin/bash
echo "The script is started:"

# function that returns a value - counts number of lines in given file
lines_in_file (){
	cat $1 | wc -l
}
num_lines=$( lines_in_file $1 )
echo "The file $1 has $num_lines lines in it".

echo "The end"
