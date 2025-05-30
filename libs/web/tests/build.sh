#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}
export CXX=$compiler
cd $JDE_DIR/Public/libs
BUILD=$JDE_DIR/Public/build/so.sh
if [ $all -eq 1 ]; then
	$BUILD $JDE_DIR/Framework/source $type $clean $compiler || exit 1;
	$BUILD crypto/src $type $clean $compiler || exit 1;
	$BUILD app/shared $type $clean $compiler || exit 1;
	$BUILD app/client $type $clean $compiler || exit 1;
	$BUILD web/client $type $clean $compiler || exit 1;
	$BUILD web/server $type $clean $compiler || exit 1;
fi
cd web/tests;
$BUILD `pwd` $type $clean $compiler || exit 1;

if [ ! -f $JDE_DIR/bin/config/Web.Tests.jsonnet ]; then ln -s `pwd`/config/Web.Tests.jsonnet $JDE_DIR/bin/config; fi;
exit $?