#!/bin/bash
sourceBuildDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [[ -z $commonBuild ]]; then source $sourceBuildDir/common.sh; fi;
baseDir="$( cd "$( dirname "$sourceBuildDir/../../../.." )" &> /dev/null && pwd )";

jdeRoot=jde;
if [ -z $JDE_DIR ]; then JDE_DIR=$baseDir/$jdeRoot; fi;
if [ -z $JDE_BASH ]; then toBashDir $JDE_DIR JDE_BASH; fi;
t=$(readlink -f "${BASH_SOURCE[0]}"); sourceBuild=$(basename "$t"); unset t;

if [[ -z "$REPO_DIR" ]]; then export REPO_DIR=$baseDir; fi;
pushd `pwd` > /dev/null;
cd $REPO_DIR

if windows; then
	findExecutable MSBuild.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/BuildTools/MSBuild/Current/Bin' 0
	findExecutable MSBuild.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/Enterprise/MSBuild/Current/Bin'
	findExecutable cmake.exe '/c/Program\ Files/CMake/bin'
	BASE_DIR='/c/Program\ Files/Microsoft\ Visual\ Studio/2022/BuildTools/VC/Tools/MSVC/';
	if [ -d "$BASE_DIR" ]; then
		MS_VERSION=`ls "$BASE_DIR" | tail -n 1`
		findExecutable cl.exe "$BASE_DIR/$MS_VERSIONbin/Hostx64/x64" 1
	else
		BASE_DIR='/c/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC';
		if [ ! -d "$BASE_DIR" ]; then echo build tools not found:  $BASE_DIR; exit 1; fi;
		MS_VERSION=`ls "$BASE_DIR" | tail -n 1`
	  findExecutable cl.exe "$BASE_DIR/$MS_VERSION/bin/Hostx64/x64" 1
		#cl.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/BuildTools/VC/Tools/MSVC/$MS_VERSION/bin/Hostx64/x64' 0
	fi;
	findExecutable vswhere.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/installer' 0
fi;

function buildLibrary {
	LIB=$1;
	CXX_FLAGS=$2;
	ARGS=$3;
	GIT_DIR=$4;
	if [[ -z "$GIT_DIR" ]]; then GIT_DIR=$LIB; fi;
	if [[ ! -z "$CXX_FLAGS" ]]; then export CMAKE_CXX_FLAGS=-DCMAKE_CXX_FLAGS="$CXX_FLAGS"; fi;
	cls;
	echo $REPO_DIR/$GIT_DIR;
	cd $REPO_DIR/$GIT_DIR;
	echo `pwd`;
	moveToDir .build;
	export CMAKE_BUILD_TYPE=RelWithDebInfo;
	export CXX=g++-13;
	export CC=gcc-13;
	export CMAKE_CXX_STANDARD=23;
	export CMAKE_POSITION_INDEPENDENT_CODE=ON;
	export CMAKE_PREFIX_PATH=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE;

	moveToDir $CXX;
	moveToDir $CMAKE_BUILD_TYPE;
	cmake -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE/$LIB -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD -DCMAKE_POSITION_INDEPENDENT_CODE=$CMAKE_POSITION_INDEPENDENT_CODE -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH $CMAKE_CXX_FLAGS $ARGS ../../..;
	#make -j;
	#make install;
}

function buildConfig
{
	config=$1;
	configClean=${2:-0};
	if [ ! -f .obj/$config/Makefile ]; then
		echo no `pwd`/.obj/$config/Makefile configClean=1;
		configClean=1;
	fi;
	cmd="$sourceBuildDir/cmake/buildc.sh `pwd` $config $configClean";
	echo $cmd
	$cmd; if [ $? -ne 0 ]; then echo "FAILED"; echo `pwd`; echo $cmd; exit 1; fi;
}

function buildLinux
{
	local=${1:-0};
	projectClean=$clean;
	if [ ! -d .obj ]; then
		mkdir .obj;
		projectClean=1;
	fi;
	if [ $local -eq 1 ]; then
	   ./buildc.sh asan $projectClean 0;
	   ./buildc.sh RelWithDebInfo $projectClean 0;
	else
		buildConfig asan $projectClean
		buildConfig RelWithDebInfo $projectClean
	fi;
}
function buildWindows2 {
	local configuration=$3;
	local cmd="$1=$configuration";
	local file=$2;
	echo buildWindows2 - cmd: $cmd file: $2;
	local out=.bin/$configuration/$file;
	#local targetDir=$baseDir/$jdeRoot/Public/stage/$configuration;
	local targetDir=.bin/$configuration;
	local target=$targetDir/$file;
	local found=0;
	if [[ -f $target ]]; then found=1; fi;
	echo $target found:  $found;
	$cmd;
	if [ $? -ne 0 ];
		then echo `pwd`;
		echo $cmd;
		exit 1;
	fi;
#		if [ -f $target ]; then echo $cmd outputing stage dir.; rm $taget; fi;
#		sourceDir=`pwd`;
#		subDir=$(if [ -d .bin ]; then echo "/.bin"; else echo ""; fi);
#		cd $targetDir;

#		cp "$sourceDir$subDir/$configuration/$file" .;
#		if [[ $file == *.dll ]]; then
#			cp "$sourceDir$subDir/$configuration/${file:0:-3}lib" .;
#		fi;
#		cd "$sourceDir";
#	else
#		echo $target - found;
#	fi;
}
function buildWindows {
	dir=$1;
	file=$2;
	echo buildWindows - dir: $dir, file: $file, clean: $clean, pwd=`pwd`;
#	if [[ ! -f "$dir.vcxproj.user" && -f "$dir.vcxproj._user" ]]; then
#		echo 'linked $dir.vcxproj.user to $dir.vcxproj._user'
#		linkFile $dir.vcxproj._user $dir.vcxproj.user;	if [ $? -ne 0 ]; then echo `pwd`; echo FAILED:  linkFile $dir.vcxproj._user $dir.vcxproj.user; exit 1; fi;
#	fi;
	if [ ${clean:-1} -eq 1 ]; then
		echo rm -r -f $dir/.build;
		rm -r -f .build;
	fi;
	if [[ -z $file ]]; then
		if [[ $dir = "Framework" ]]; then
			file="Jde.dll";
		elif [[ $dir == Jde* ]]; then
			file="$dir.dll";
			echo file: $file;
		else
			file="Jde.$dir.dll";
		fi;
	fi;
	projFile=$([ "${PWD##*/}" = "tests" ] && echo "Tests.$dir" || echo "$dir");
	baseCmd="msbuild.exe $projFile.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
	buildWindows2 "$baseCmd" $file release;
	buildWindows2 "$baseCmd" $file debug;
	echo build $projFile complete.
}

function createProto {
	dir=$1;
	file=$2;
	export=$3;
	cleanProtoc=$clean;
	pushd `pwd` > /dev/null;
	cd $dir;
	if [ ! -f file.pb.cc ]; then
		cleanProtoc=1;
	fi;
	if [ $cleanProtoc -eq 1 ]; then
		protoc --cpp_out . $file.proto;
		if [ $? -ne 0 ]; then exit 1; fi;
	fi;
	popd > /dev/null;
}

function fetchDefault {
	cd $baseDir/$jdeRoot;
	fetch $1 $2;
	return $?;
}
#$1 - directory, $2 - whether to use local build file, $3 - resulting dll/exe.
function build {
	if windows; then
		buildWindows $1 $3
	else
		buildLinux $2
	fi;
}
function buildCMake {
	if windows; then
		moveToDir .build;
		cmake -DVCPKG=ON -Wno-dev ..; if [ $? -ne 0 ]; then exit $LINENO; fi;
		build $1 $2 $3;
	else
		echo "Not implemented";
		exit 1;
	fi;
}



function buildTest {
	if windows; then
		MSBuild.exe -t:restore -p:RestorePackagesConfig=true
		if [[ ! -f "Tests.$dir.vcxproj.user" && -f "Tests.$dir.vcxproj._user" ]]; then
			linkFile Tests.$dir.vcxproj._user Tests.$dir.vcxproj.user;	if [ $? -ne 0 ]; then echo `pwd`; echo FAILED:  linkFile Tests.$dir.vcxproj._user Tests.$dir.vcxproj.user; exit 1; fi;
		fi;

		buildWindows $1 $2
	else
		buildLinux 1
	fi;
}
function fetchBuild {
	fetchDefault $1;
	build $1 $2 $3;
}
function findProtoc {
	if [ -z $PROTOBUF_INCLUDE ]; then
		PROTOBUF_INCLUDE=$REPO_BASH/protobuf/src/google/protobuf;
	fi;
	if windows; then
		findExecutable protoc.exe $JDE_BASH/Public/stage/release;
	fi;
}
popd > /dev/null;