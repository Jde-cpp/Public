#Function bodies are subshells - ( ) not { } - so sourcing callers (CLAUDE.md flow, VS Code tasks) don't inherit cd/set -o pipefail/variables.
function buildRelativePath() (
	cmakeSourceDir=$1; #/home/duffyj/code/jde/Public2
	absoluteFile=$2; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	relativePath=${absoluteFile#"$cmakeSourceDir/"};
	relativePath=${relativePath%/*};
	filename=$(basename "$absoluteFile");
	if [[ $filename == "main.cpp" ]]; then sub=exe; else sub=lib; fi;
	relativePath=${relativePath/\/src\///$sub/}; #component-anchored: mid-path /src/ …
	relativePath=${relativePath/%\/src//$sub};   #… or trailing /src - never a substring like foosrc.
	echo $relativePath;
)
function absoluteFile() (
	workspaceFolder=$1;
	relativeFile=$2;
	absoluteFile=`realpath $workspaceFolder/$relativeFile`; #/home/duffyj/code/jde/Framework/source/io/DiskWatcher.cpp
	echo $absoluteFile;
)
function buildProject() (
	cmakeSourceDir=$(realpath $2); #/home/duffyj/code/jde/PublicX
	# buildRoot layout `$1/<repo-basename>` (`$1`=`$JDE_BUILD_DIR/$JDE_COMPILER`) is mirrored in TS by
	# extensions/jde/src/extension.ts repoBuildDir(); a change here must update both. Repeated in compile/reconfig/build.
	buildRoot=$1/$(basename $cmakeSourceDir); # /mnt/ram/jde/clang++/PublicX
	workspaceFolder=$3; #/home/duffyj/code/jde/IotWebsocket/config
	relativeFile=$4; #../tests/BrowseTests.cpp
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	buildRelativePath=`buildRelativePath $cmakeSourceDir $absoluteFile`;

	#The per-directory Makefiles already know their targets; walk up to the nearest one for files in nested source dirs.
	buildDir=$buildRoot/$buildRelativePath;
	while [[ $buildDir != $buildRoot && ! -f $buildDir/Makefile ]]; do buildDir=${buildDir%/*}; done;
	if [[ $buildDir == $buildRoot ]]; then
		echo "buildProject: no Makefile found under $buildRoot/$buildRelativePath - not a configured source dir.";
		return 1;
	fi;
	echo cmakeSourceDir=$cmakeSourceDir, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, buildDir=$buildDir;
	make -C $buildDir -j$(nproc) 2>&1;
)
function compile() (
	buildRoot=$1; # /mnt/ram/jde/clang++/PublicX/debug
	file=$(realpath "$2"); #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	cache=$buildRoot/CMakeCache.txt;
	if [ ! -f "$cache" ]; then
		echo "compile: no CMakeCache.txt in $buildRoot - run reconfig first.";
		return 1;
	fi;
	#the cache is authoritative for the source dir; deriving it from $buildRoot's basename breaks whenever the
	#build dir and the checkout are named differently.
	repoSourceDir=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$cache");
	relativeFile=${file#"$repoSourceDir/"}; #libs/fwk/src/io/json.cpp
	if [[ $relativeFile == "$file" ]]; then
		echo "compile: $file is not under $repoSourceDir.";
		return 1;
	fi;

	#Only add_subdirectory() dirs get a build dir - nested source dirs (libs/ql/ops, libs/web/server/auth) don't,
	#so walk up to the nearest Makefile.  buildRelativePath supplies the src->lib/exe rename first.
	buildDir=$buildRoot/`buildRelativePath $repoSourceDir $file`;
	while [[ $buildDir != $buildRoot && ! -f $buildDir/Makefile ]]; do buildDir=${buildDir%/*}; done;
	if [[ $buildDir == $buildRoot ]]; then
		echo "compile: no Makefile for $relativeFile under $buildRoot - not a configured source dir.";
		return 1;
	fi;

	#The Makefile is authoritative for the object target: its name is relative to the CMakeLists that owns the
	#file, which isn't derivable from the path - e.g. apps/<App>/CMakeLists.txt lists src/main.cpp, so the rule
	#in apps/<App>/exe is src/main.cpp.o, not main.cpp.o.  Longest path-anchored suffix match wins.
	obj=$(grep -oE '^[A-Za-z0-9_/.+-]+\.o:' $buildDir/Makefile | sed 's/:$//' | awk -v f="$relativeFile" '
		{ o=$0; sub(/\.o$/, "", o); n=length(f); m=length(o);
			if( n>=m && substr(f, n-m+1)==o && (n==m || substr(f, n-m, 1)=="/") && m>bestLen ){ best=$0; bestLen=m } }
		END{ if( bestLen ) print best }' );
	if [ -z "$obj" ]; then
		echo "compile: no object rule for $relativeFile in $buildDir/Makefile - not a source of any target there.";
		return 1;
	fi;
	cd $buildDir || return 1;
	echo "make -C $buildDir $obj";
	make $obj 2>&1;
)
function reconfig() (
	buildDir=$1;
	repoSourceDir=$(realpath $2);
	preset=$3;
	echo "reconfig repoSourceDir=$repoSourceDir buildDir=$buildDir preset=$preset";
	mkdir -p $buildDir/runtime/logs;
	cd $buildDir || return 1;
	rm -f CMakeCache.txt;
	echo `pwd` | tee cmake.output;
	echo "cmake -B $buildDir -S $repoSourceDir -Wno-dev --preset "$preset" 2>&1" | tee -a cmake.output;
	#-B required: no Linux preset sets binaryDir, and preset mode ignores cwd (defaults to the source dir).
	cmake -B $buildDir -S $repoSourceDir -Wno-dev --preset "$preset" 2>&1 | tee -a cmake.output;
	if [ -f "$buildDir/compile_commands.json" ]; then
		mv $buildDir/compile_commands.json $repoSourceDir/compile_commands.json;
	fi
)
function build() (
	repoBuildDir=$1;
	repoSourceDir=$(realpath $2);
	target=$3;
	cd $repoBuildDir || return 1;
	set -o pipefail;
	echo `pwd`/$target.output | tee $target.output;
	echo "cmake --build . -j --target $target" | tee -a $target.output;
	cmake --build . -j --target $target 2>&1 | tee -a $target.output;
)
function clean() (
	repoSourceDir=$(realpath $2);
	repoBuildDir=$1/$(basename $repoSourceDir); #scope to this checkout's build dir - a find from $1 would sweep every checkout's PCHs.
	cd $repoBuildDir || return 1;
	cmake --build . --target clean;
	find . -name 'cmake_pch.hxx.pch' -print -delete;
)
function buildTests() (
	baseTarget=$3;
	if [ $baseTarget == "Jde.Opc.Gateway" ]; then
		baseTarget="Jde.Opc";
	fi;
	build $1 $2 $baseTarget.Tests;
)
