#!/bin/bash
buildTarget=${1:-Debug};#relWithDebInfo
clean=${2:-0};
buildDir=${3:-$JDE_BUILD_DIR};

sourceDir=$JDE_DIR/Public;

windows() { [[ -n "$WINDIR" ]]; }

if windows; then
	preset="win-jde-${buildTarget,,}";
else
	if [ $buildTarget == "Debug" ]; then
		preset="linux-jde-debug";
	else
		preset="linux-jde-"${buildTarget,};
	fi
fi

function copyConfigFile {
	local lib=$1;
	local capitalized=${lib^};
	if ! windows; then
		if ! [ -L ~/.Jde-Cpp/Tests.$capitalized/$capitalized.Tests.jsonnet ]; then
			mkdir -p ~/.Jde-Cpp/Tests.$capitalized;
			echo $sourceDir/libs/$lib/tests/config/$capitalized.Tests.jsonnet;
			ln -s $sourceDir/libs/$lib/tests/config/$capitalized.Tests.jsonnet ~/.Jde-Cpp/Tests.$capitalized;
		fi
	fi
}

function test {
	local lib=$1;
	copyConfigFile $lib;
	mkdir -p $buildDir/$buildTarget/libs/$lib;
	cd $buildDir/$buildTarget/libs/$lib;
	if [ $clean -eq 1 ]; then
		rm -rf *;
	fi
	if [ ! -f CMakeCache.txt ]; then
		cmake $sourceDir/libs/$lib --preset $preset;
	fi
	make -j; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
	ctest --verbose; if [ $? -ne 0 ]; then echo "Tests Failed"; echo `pwd`; exit 1; fi;
}

test crypto
test access;
test web;

