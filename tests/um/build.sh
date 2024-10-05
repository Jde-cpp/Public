#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}
export CXX=$compiler
cd $JDE_DIR/Public/
BUILD=$JDE_DIR/Public/build/so.sh
if [ $all -eq 1 ]; then
	$BUILD $JDE_DIR/Framework/source $type $clean $compiler || exit 1;
	$BUILD src/db $type $clean $compiler || exit 1;
	$BUILD src/ql $type $clean $compiler || exit 1;
	$BUILD src/um $type $clean $compiler || exit 1;
	export Boost_INCLUDE_DIR=/home/duffyj/code/libraries/boostorg/boost_1_86_0;
	$BUILD $JDE_DIR/MySql/source $type $clean $compiler || exit 1;
fi

cd tests/um;
$BUILD `pwd` $type $clean $compiler || exit 1;

# if [ ! -f $JDE_DIR/bin/config/Tests.UM.json ]; then ln -s `pwd`/config/Tests.UM.json $JDE_DIR/bin/config; fi;
exit $?