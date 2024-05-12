#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}
export CXX=$compiler;
if [ $all -eq 1 ]; then
	../../build/so.sh ../../../Framework/source $type $clean || exit 1;
fi
if [ ! -d .obj ];	then
	mkdir .obj;
	clean=1;
fi;
cd .obj;
if [ ! -d $type ]; then
	mkdir $type;
	clean=1;
fi;
cd $type;
if [ $clean -eq 1 ]; then
	rm -f CMakeCache.txt;
	cmake -DCMAKE_BUILD_TYPE=$type  ../.. > /dev/null;
	make clean;
fi
make -j
cd - > /dev/null
exit $?
