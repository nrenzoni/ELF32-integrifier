#!/bin/bash

# output: intel formatted 32 bit asm for arg c file

if [[ $# -lt 1 ]]; then
	echo "usage: <file.c for 32 asm output> [gcc optimization level (0-3)]"
	exit
fi

if [[ ! -r $1 ]]; then
	echo "$1 is not a readable file"
	exit
fi

if [[ $1 != *.c ]]; then
	echo "$1 does not contain a .c suffix"
	exit
fi

# if 2nd arg is assigned and it is a positive number (should be between 0-3)
if [[ ( ! -z $2 ) && ( $2 =~ ^[0-9]+$ ) ]]; then
	optim_level=$2
else
	optim_level=0
fi

# remove path from path-string
basename=${1##*/}
# remove extension
basename=${basename%%.*}

# output intel formatted 32 bit asm for arg c file
gcc -S -O${optim_level} -fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables -fverbose-asm -m32 -masm=intel $1 -I ./../include/ -o ${basename}.s
echo asm output to: ${basename}.s
echo optimization level: $optim_level
