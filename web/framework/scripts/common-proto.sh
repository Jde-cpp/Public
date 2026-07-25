#!/bin/bash
function create {
	dir=$1;
	local -n files=$2;
	wsFolder=$3;
	#cwd is the proto output dir here, not the workspace, so wsFolder cannot be defaulted from it - the callers cd to
	#the workspace and pass it.  Empty would silently probe /node_modules/.bin/pbjs, so say so instead.
	if [ -z "$wsFolder" ]; then echo `pwd`; echo createProtos: no workspace dir passed; exit 1; fi;
	echo createProtos pwd=`pwd` dir=$dir files=$2 wsFolder=$wsFolder;
	for i in "${!files[@]}"; do
		if [ ! -f $i.proto ]; then
			mklink $i.proto $dir;
		fi;
	done;
	#pushd `pwd` > /dev/null;
	#cd $wsFolder;
	local pbjs=$wsFolder/node_modules/.bin/pbjs;
	local pbts=$wsFolder/node_modules/.bin/pbts;
	#do not use npx - these dirs are symlinked out of the workspace, so npm resolves its local prefix outside it and fetches the unrelated registry 'pbjs' package.
	if [ ! -x $pbjs ] || [ ! -x $pbts ]; then echo $PS4 protobufjs-cli not found in $wsFolder/node_modules; exit 1; fi;
	for i in "${!files[@]}"; do
		echo $pbjs -r ${files[$i]} -t static-module -w es6 -o `pwd`/$i.js `pwd`/$i.proto;
		$pbjs -r ${files[$i]} -t static-module -w es6 -o `pwd`/$i.js `pwd`/$i.proto; if [ $? -ne 0 ]; then echo `pwd`; echo $pbjs -r ${files[$i]} -t static-module -w es6 -o `pwd`/$i.js `pwd`/$i.proto; exit 1; fi;
		$pbts -o `pwd`/$i.d.ts `pwd`/$i.js; if [ $? -ne 0 ]; then echo `pwd`; echo $pbts -o `pwd`/$i.d.ts `pwd`/$i.js; exit 1; fi;
	done;
	#popd > /dev/null;
#	for i in "${!files[@]}"; do
#		rm $i.proto;
#	done;
}