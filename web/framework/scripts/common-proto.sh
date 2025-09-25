#!/bin/bash
function create {
	dir=$1;
	local -n files=$2;
	for i in "${!files[@]}"; do
		if [ ! -f $i.proto ]; then
			mklink $i.proto $dir;
		fi;
	done;
	for i in "${!files[@]}"; do
		echo npx pbjs -r ${files[$i]} -t static-module -w es6 -o $i.js $i.proto;
		npx pbjs -r ${files[$i]} -t static-module -w es6 -o $i.js $i.proto;npx pbts -o $i.d.ts $i.js;
	done;
#	for i in "${!files[@]}"; do
#		rm $i.proto;
#	done;
}