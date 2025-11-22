#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}
export CXX=$compiler
cd $JDE_DIR/
BUILD=$JDE_DIR/build/so.sh
if [ $all -eq 1 ]; then
	$BUILD $JDE_DIR/Framework/source $type $clean $compiler || exit 1;
	$BUILD libs/db/src $type $clean $compiler || exit 1;
	export Boost_INCLUDE_DIR=/home/duffyj/code/libraries/boostorg/boost_1_86_0;
	$BUILD libs/db/src/drivers/mysql $type $clean $compiler || exit 1;
	$BUILD libs/ql $type $clean $compiler || exit 1;
	$BUILD libs/access/src $type $clean $compiler || exit 1;
fi
echo `pwd`;
cd libs/access/tests;
$BUILD `pwd` $type $clean $compiler || exit 1;

# if [ ! -f $JDE_DIR/bin/config/Tests.UM.json ]; then ln -s `pwd`/config/Tests.UM.json $JDE_DIR/bin/config; fi;
exit $?