#!/bin/bash

# script should be run from directory containing src and include directories

# first arg should be name of file to replace without extension
# second arg should be new name without extension

if [[ $# -ne 2 ]]; then
	echo "usage: $0 <previous_name_without_extension> <new_name_without_extension>"	
	exit
fi

# swaps names in header include declarations. ignores vim .swp files
find src/ include/ -type f ! -name '*.swp' -exec sed -i "s/$1/$2/" {} ';'

# rename preserves extensions
rename 's/$1/$2/' */$1*
