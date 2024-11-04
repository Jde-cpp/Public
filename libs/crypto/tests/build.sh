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
	echo `pwd`;
	$BUILD libs/crypto/src $type $clean $compiler || exit 1;
fi
$BUILD libs/crypto/tests $type $clean $compiler || exit 1;

#if [ ! -f $JDE_DIR/bin/config/Crypto.Tests.jsonnet ]; then
#	ln -s $JDE_DIR/Public/libs/crypto/tests/config/Crypto.Tests.jsonnet $JDE_DIR/bin/config/Crypto.Tests.jsonnet;
#fi;
exit $?