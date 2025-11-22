#!/bin/bash
buildRepos=${1:-0}
buildTests=${2:-1}
runTests=${3:-1}
clearCache=${4:-0}
releaseRoot=$JDE_BASH/Release/g++-14
sourceDir=$JDE_BASH/Public
set -e

function buildLib() {
  projectDir=$1;
	buildDir=$2;
	echo mkdir -p $buildDir;
	mkdir -p $buildDir;
	cd $buildDir;

	if [ $clearCache -eq 1 ] || [ ! -e CMakeCache.txt ]; then
		rm -f CMakeCache.txt;
		cmake $projectDir --preset linux-jde-relWithDebInfo $defines;
	fi;
	echo `pwd`;
	echo make -j;
	make -j;
	if [ $? -ne 0 ]; then echo $lib Failed; echo `pwd`; echo "make -j"; exit 1; fi;
}
function buildStdLib() {
	lib=$1;
	buildLib $sourceDir/libs/$lib $releaseRoot/libs/$lib;
}
function buildLibTests() {
	buildStdLib $1;
	if [[ $runTests -eq 1 ]]; then
		echo -------------------------Run $lib--------------------------
		cd tests;
		export LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH;
		echo `pwd`;
		echo ctest --output-on-failure;
		ctest --output-on-failure;
		if [ $? -ne 0 ]; then echo "Tests Failed"; echo `pwd`; echo ctest --output-on-failure; exit 1; fi;
	fi;
}

if [[ $buildRepos -eq 1 ]]; then
	echo ------------------------Build Repos-------------------------
	mkdir -p $releaseRoot/repos;
	cd $releaseRoot/repos;
	if [[ $clearCache -eq 1 ]]; then
		rm -f CMakeCache.txt;
		cmake $sourceDir/build --preset linux-relWithDebInfo;
	fi;
	cmake --build . --config RelWithDebInfo --target all -- -j;
fi;
defines=-Djde_BUILD_TESTS=$([ $buildTests == 1 ] && echo "ON" || echo "OFF" );
if [[ $buildTests -eq 1 ]]; then
	echo -------------------------Build Tests-------------------------
	#buildLibTests fwk;
	#buildLib $sourceDir/libs/db/src $releaseRoot/libs/db/lib;
	buildLib $sourceDir/libs/db/drivers/mysql $releaseRoot/libs/db/drivers/mysql;
	buildLibTests access;
fi;
exit 0;
echo -------------------------Build Apps--------------------------
app=AppServer;
appDir=$releaseRoot/apps/$app;
mkdir -p $appDir;
cd $appDir;
if [[ $clearCache -eq 1 ]]; then
	rm -f CMakeCache.txt;
	cmake $sourceDir --preset linux-jde-relWithDebInfo;
fi;
sourceDir=$JDE_DIR/apps/$app;
make -j;