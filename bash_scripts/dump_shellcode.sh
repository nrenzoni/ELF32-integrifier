#!/bin/bash

# input file as arg to script containing only shellcode bytes
# output of shellcode bytes as hexbytes to stdout

if [[ $# -lt 1 ]]; then
	echo "usage: $0 <shellcode file>"
	exit
fi

if [[ ! -r $1 ]]; then
	echo "$1 is not a readable file"
	exit
fi

file_size=$(wc -c $1 | cut -d' ' -f1)

od --address-radix=n --width=${file_size} --output-duplicates --format=x1 $1
