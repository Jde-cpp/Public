#!/bin/bash
#sudo npm install -g @angular/cli@latest
#sudo npm install -g npm@latest
#sudo apt  install jq
#chmod 777 ../WebFramework/jde-framework-proto.sh
#run from my-workspace/..
#../WebFramework/create-workspace.sh my-workspace MaterialSite WebFramework TwsWebsite WebBlockly
workspace=${1:-my-workspace};
librariesText=( "$@" );
declare -a libraries=();
for i in "${!librariesText[@]}"; do if (( $i > 0 )); then t=${librariesText[$i]}; libraries+=($t); libraryLog="$libraryLog $t";  fi; done;
echo create-workspace.sh workspace=$workspace libraries=\"${libraryLog:1}\";

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
echo $scriptDir;
source $JDE_BASH/build/common.sh;
baseDir=`pwd`;
echo $baseDir;
#REPO_WEB=`readlink -f $scriptDir/../..`;
findExecutable npm;
if [ ! -x "$(which "ng" 2> /dev/null)" ]; then
	cmd="npm install -g @angular/cli";
	echo d;
	if windows; then
		$cmd;
	else
		sudo $cmd;
	fi;
	if [ $? -ne 0 ]; then echo `pwd`; echo $cmd; exit 1; fi;
fi;
##################
#`ng new` emits tsconfig.json with leading /* */ comment lines, which jq cannot parse.  Strip whole-line
#comments before piping to jq - matching create-library.sh - so a // inside a string value is left alone.
#Only replace the original once jq has succeeded: `cmd > tmp; rm orig; mv tmp orig` zeroed the file on any failure.
function jqEdit() {
	local file=$1; local filter=$2;
	if [ ! -f "$file" ]; then echo `pwd`; echo "jqEdit: $file not found"; exit 1; fi;
	sed -e '/^[[:space:]]*\/\*/d' -e '/^[[:space:]]*\*/d' -e '/^[[:space:]]*\/\//d' "$file" | jq "$filter" > "$file.tmp";
	#jq exits 0 on empty input, so check for output too - otherwise an already-zeroed file rewrites as still-zeroed.
	if [ ${PIPESTATUS[1]} -ne 0 ] || [ ! -s "$file.tmp" ]; then echo `pwd`; echo jq "$filter" $file; rm -f "$file.tmp"; exit 1; fi;
	mv "$file.tmp" "$file";
}
##################
if [ ! -d $workspace ]; then
	echo -------------------- create workspace start --------------------;
	#the workspace must come out on the same Angular as the global cli that scaffolds it.  `ng new` already writes
	#its own version, but the installs below default to @latest and would drag a newer major into a workspace built
	#by an older cli.  Read the version off the package.json next to the resolved ng, not `ng --version` output.
	ngPath=`readlink -f "$(which ng)"`;
	ngVersion=`node -p "require('$(dirname $ngPath)/../package.json').version" 2> /dev/null`;
	if [ -z "$ngVersion" ]; then echo `pwd`; echo could not read the global @angular/cli version near $ngPath; exit 1; fi;
	ngMajor=${ngVersion%%.*};
	echo global @angular/cli $ngVersion - creating the workspace to match;
	createApplication=true;
	cmd="ng new $workspace --create-application=$createApplication --ai-config=claude --defaults --routing=false --style=scss"
	$cmd; if [ $? -ne 0 ]; then echo $cmd; exit 1; fi;
	echo -------------------- create workspace complete --------------------;
	cd $workspace;
	#claude code reads .mcp.json from the workspace root, so give the workspace its own angular cli mcp server
	#(list_projects, get_best_practices, search_documentation, ai_tutor).  pinned for the same reason as the installs
	#below - an unpinned npx fetches @latest and would answer for a cli the workspace was not built with.
	cat > .mcp.json <<-EOF
	{
	  "mcpServers": {
	    "angular-cli": { "command": "npx", "args": ["-y", "@angular/cli@$ngVersion", "mcp"] }
	  }
	}
	EOF
	if [ $? -ne 0 ]; then echo `pwd`; echo could not write .mcp.json; exit 1; fi;
	jqEdit angular.json ".projects.\"$workspace\".architect.build.configurations.production.budgets[0].maximumError = \"2mb\"";
	jqEdit angular.json ".projects.\"$workspace\".architect.build.configurations.production.budgets[0].maximumWarning = \"1mb\"";
	#was a sed on '"strict": true,' - Angular 22's --defaults template emits the individual noImplicit* flags and no
	#"strict" key at all, so it matched nothing and exited 0.  These are the tsc defaults today (strict unset), but
	#state them so the libraries keep building if a future template turns strict on.
	jqEdit tsconfig.json '.compilerOptions += {"strictPropertyInitialization":false, "strictNullChecks":false, "noImplicitAny":false, "noImplicitThis":false, "allowSyntheticDefaultImports":true}';
	jqEdit angular.json ".cli.analytics = false";
	#library sources are symlinked in from web/<lib>/control below - without preserveSymlinks tsc and esbuild resolve them to their real paths outside the workspace, which breaks the compilation and the sass node_modules lookup.
	jqEdit angular.json ".projects.\"$workspace\".architect.build.options.preserveSymlinks = true";
	jqEdit tsconfig.json ".compilerOptions.preserveSymlinks = true";
	#ng analytics disable;
	echo -------------------- npm install start --------------------;
	npm install material-design-icons;
	#@angular/animations is not installed: material 22 has no dependency on it, nothing here uses the animations
	#metadata api, and the package is deprecated in v22 in favour of animate.enter/animate.leave.
	echo -------------------- icons installed --------------------;
	#material/cdk have their own patch cadence (no @angular/material 22.0.8 exists when the cli is 22.0.8), so pin
	#the major rather than $ngVersion.
	ng add @angular/material@$ngMajor --defaults --skip-confirmation; #use skip-confirmation when interactive
	echo -------------------- material installed --------------------;
	#echo `pwd`;
	#exit 1;
	npm install @types/long --save;
	npm --silent install chalk@^4.0.0;
	npm --silent install jsdoc@^3.6.3;
	npm --silent install uglify-js@^3.7.7;
	echo -------------------- npm install complete --------------------;
	echo `pwd`;
	cd src;
	printf "\nimport * as protobuf from 'protobufjs/minimal';\nimport Long from 'long';\n\nprotobuf.util.Long = Long;\nprotobuf.configure();" >> main.ts;
	cd ..;
else
	echo workspace already exists
	cd $workspace;
fi;
pushd `pwd` > /dev/null;
##################
function execute() {
	file=$1;
	if [ -f  $file ];then
		t=$(stat -c "%a %n" $file); if [[ ${t:0:1} != "7" ]] ; then chmod 7${t:1:2} $file; fi;
		$file $2; if [ $? -ne 0 ]; then echo `pwd`; echo $file $2; exit 1; fi;
	fi;
}
##################
startDir=`pwd`;
echo -------------------- Start Libraries --------------------;
for libraryDir in "${libraries[@]}"; do
	echo $libraryDir - processing;
	#findExecutable jq
	#-e so jq exits non-zero when .name is absent - plain `jq -r` prints the string "null" and exits 0, and the
	#assignment on the next line would clobber the status anyway, so $? was only ever testing basename.
	dest=$(jq -er .name $libraryDir/control/package.json) || { echo `pwd`; echo no .name in $libraryDir/control/package.json; exit 1; };
	#dest=`perl -MJSON -e '$/ = undef; my $data = <>; for my $hash (new JSON->incr_parse($data)) { print $hash->{name}; }' < $libraryDir/control/package.json`;
	library=$( basename $dest );
	if [ ! -d projects/$library ]; then
		$scriptDir/create-library.sh $library $libraryDir; if [ $? -ne 0 ]; then echo `pwd`; echo $scriptDir/WebFramework/create-library.sh $library $libraryDir; exit 1; fi;
	fi;
	#unchecked, a failed cd left cwd at the workspace root and the rm -rf below ran there instead.
	cd $baseDir/$workspace/projects/$library/src; if [ $? -ne 0 ]; then echo `pwd`; echo cd $baseDir/$workspace/projects/$library/src; exit 1; fi;
	#public-api.ts is the library's export surface.  It used to be hard-linked by create-library.sh, which only runs
	#when projects/$library is absent - so once a replace-and-rename write (editor save, sed -i, git checkout) broke
	#the link, the workspace copy froze and re-running setup could not repair it.  Symlink it alongside lib/, on every
	#run.  mklink (build/common.sh) drops any existing file or stale symlink first and carries the windows branch.
	if [ ! -f $libraryDir/control/src/public-api.ts ]; then echo `pwd`; echo $libraryDir/control/src/public-api.ts not found; exit 1; fi;
	mklink public-api.ts $libraryDir/control/src; if [ $? -ne 0 ]; then echo `pwd`; echo mklink public-api.ts $libraryDir/control/src; exit 1; fi;
	#mklinkDir, not a raw ln -s: build/common.sh carries the windows() branch (cmd mklink /D), which a bare ln -s
	#silently loses - on Git Bash it writes an unusable ~120-byte stub file instead of a link.
	if [ -d $libraryDir/control/src/lib ]; then
		#addHardDir lib $libraryDir/control/src;
		#only a real directory needs the recursive delete.  Unlinking a symlink must never go through rm -rf, where a
		#stray trailing slash (`rm -rf lib/`) follows the link and empties the source tree instead of removing it.
		if [ -L lib ]; then rm lib; elif [ -d lib ]; then echo removing workspace lib directory; rm -rf ./lib; fi;
		echo mklinkDir lib $libraryDir/control/src;
		mklinkDir lib $libraryDir/control/src;
	fi;
	if [ -d $libraryDir/control/src/assets ]; then
		#addHardDir assets $libraryDir/control/src;
		mklinkDir assets $libraryDir/control/src;
	fi;
	if [ -d $libraryDir/control/src/styles ]; then
		#addHardDir styles $libraryDir/control/src; fi;
		mklinkDir styles $libraryDir/control/src;
	fi;
	cd $baseDir/$workspace;
	execute $libraryDir/scripts/$library.sh;
	echo execute $libraryDir/scripts/$library-proto.sh $baseDir/$workspace;
	execute $libraryDir/scripts/$library-proto.sh $baseDir/$workspace;
	#if release-mode then
	#ng build $library; if [ $? -ne 0 ]; then echo `pwd`; echo ng build $library; exit 1; fi;
	#cd dist/$library; npm pack;
	cd $startDir
done;
echo -------------------- End Libraries --------------------;
echo create-workspace.sh success