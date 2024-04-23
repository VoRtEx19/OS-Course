#!/bin/bash
echo "The script is started:"

# function that takes arguments
print_hello() {
	# $1 is 1st argument
	echo "Hello, $1!"
}

# say hi to Sun and Moon
print_hello Sun
print_hello Moon

echo "The end"
