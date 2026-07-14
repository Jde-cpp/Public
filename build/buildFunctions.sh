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
	cmakeSourceDir=$(realpath $2); #/home/duffyj/code/jde/PublicX
	buildRoot=$1/$(basename $cmakeSourceDir); # /mnt/ram/jde/clang++/PublicX
	workspaceFolder=$3; #/home/duffyj/code/jde/IotWebsocket/config
	fileWorkspaceFolder=$4; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests
	relativeFile=$5; #../tests/BrowseTests.cpp
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	buildRelativePath=`buildRelativePath $cmakeSourceDir $absoluteFile`; #libs/web/server

	echo "cmakeSourceDir=$cmakeSourceDir, buildRoot=$buildRoot, workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile";
	cd $buildRoot/$buildRelativePath || return 1;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"} #shortest-match prefix removal
	filename=$(basename "$absoluteFile");
	if [[ $filename == "main.cpp" ]]; then
		buildFile=src/main.cpp;
	fi;
	echo $buildRoot/$buildRelativePath/make $buildFile.o;
	make ${buildFile}.o 2>&1;
)
function reconfig() (
	cmakeDir=$(realpath $2);
	buildRoot=$1/$(basename $cmakeDir);
	preset=$3;
	mkdir -p $buildRoot/runtime/logs;
	cd $buildRoot || return 1;
	rm -f CMakeCache.txt;
	echo `pwd` > cmake.output;
	echo "cmake $cmakeDir -B $buildRoot -Wno-dev --preset $preset 2>&1" | tee -a cmake.output;
	cmake $cmakeDir -B $buildRoot -Wno-dev --preset "$preset" 2>&1 | tee -a cmake.output; #-B required: no Linux preset sets binaryDir, and preset mode ignores cwd (defaults to the source dir).
	if [ -f "$buildRoot/compile_commands.json" ]; then
		mv $buildRoot/compile_commands.json $cmakeDir/compile_commands.json;
	fi
)
function build() (
	repoSourceDir=$(realpath $2);
	repoBuildDir=$1/$(basename $repoSourceDir);
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
