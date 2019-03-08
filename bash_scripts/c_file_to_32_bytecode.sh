#!/bin/bash

if [[ $# -lt 1 ]];then
	echo usage: <file.c> [optimization level 0-3]
	exit
fi

# if 2nd arg is assigned and it is between 0-3
if [[ ( ! -z $2 ) && ( $2 =~ ^[0-3]$ ) ]]; then
	optim_level=$2
else
	optim_level=0
fi

# remove path from path-string
basename=${1##*/}
# remove extension
basename=${basename%%.*}

my_dir="$(dirname "$(readlink -f "$0")")"

gcc -m32 -O${optim_level} -c $1 -o ${basename}.o

if [[ ! $? -eq 0 ]]; then
	echo error in gcc, quiting script
	exit 1
fi

# output will be ${basename}_txt_section
${my_dir}/extract_txt_section_bytes.sh ${basename}.o

if [[ ! $? -eq 0 ]]; then
	echo error in extract_txt_section_bytes.sh
fi

rm ${basename}.o

