#!/bin/bash
path=${1}
type=${2:-asan} #asan, RelWithDebInfo, debug, release
clean=${3:-0}
compiler=${4:-g++-13}

dir=../..;
cwd=`pwd`
export CXX=$compiler;
cd $path
if [ ! -d .obj ]; then mkdir .obj; fi;
cd .obj;
if [ ! -d $type ]; then mkdir $type; fi;
cd $type
echo `pwd`;
if (( $clean == 1 )) || [ ! -f CMakeCache.txt ]; then
	echo cleaning $path;
	if [ -f CMakeCache.txt ]; then rm CMakeCache.txt; fi;
	cmake -DCMAKE_BUILD_TYPE=$type $dir > /dev/null;
	make clean;
fi
make -j;
result=$?;
cd $cwd;
exit $result;