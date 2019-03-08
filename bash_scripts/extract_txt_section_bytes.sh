#!/bin/bash

# input:  in_file.o as arg of obj file containing .text section
# output: in_file_txt_section containing only bytes from .text section

if [[ $# -lt 1 || ! -r $1 ]]; then
	echo "usage: $0  <elf file cotaining .text section>"
	exit
fi

# remove path from path-string
basename=${1##*/}
# remove extension
basename=${basename%%.*}


start_off_and_len=$(readelf -S $1 | grep .text | sed 's/ \+/ /g' | cut -d' ' -f7,8)

arr=($start_off_and_len)
start_offset=${arr[0]}
len=${arr[1]}

if [[ -z $start_offset || -z $len ]]; then
	echo "error: bad output from readlef -S $1"
	exit
fi

# echo "dd skip=$((0x${start_offset})) count=$((0x${len})) if=$1 of=${basename}_txt_section bs=1"
# echo "output to: ${basename}_txt_section" 
dd skip=$((0x${start_offset})) count=$((0x${len})) if=$1 of=${basename}_txt_section bs=1 status=none
