#!/bin/bash

#Collects $1 and, transitively, every .proto it imports into the global $_protoDeps.  Imports are matched by basename
#against the cwd - the output dir - because that is where create() symlinks everything pbjs has to resolve; the
#google/protobuf/* ones it ships itself are not linked in, so a name that is not there is simply not a dependency.
function protoDeps {
	local proto=$1;
	if [ -n "${_protoDeps[$proto]}" ] || [ ! -f $proto ]; then return 0; fi;
	_protoDeps[$proto]=1;
	local import;
	for import in `sed -n 's/^[[:space:]]*import[[:space:]]*"\([^"]*\)".*/\1/p' $proto`; do
		protoDeps `basename $import`;
	done;
}

#true when $1.js/$1.d.ts still have to be built - they are missing, or a .proto they were generated from has changed
#since.  pbjs emits a static module with the imported messages inlined, so an import's timestamp counts as much as
#the target's own: Opc.FromServer.d.ts goes stale when Opc.Common.proto moves under it, not just when its own does.
#Only call this from an if/&&/|| condition - env.sh traps ERR, and the `return 1` here is an answer, not a failure.
function isStale {
	local name=$1;
	if [ ! -f $name.d.ts ] || [ ! -f $name.js ]; then return 0; fi;
	unset _protoDeps; declare -gA _protoDeps;
	protoDeps $name.proto;
	local proto;
	for proto in "${!_protoDeps[@]}"; do
		if [ $proto -nt $name.d.ts ]; then return 0; fi;
	done;
	return 1;
}

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