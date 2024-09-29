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
#	$BUILD $JDE_DIR/Ssl/source $type $clean $compiler || exit 1;
	$BUILD src/crypto $type $clean $compiler || exit 1;
#	$BUILD src/app/client $type $clean $compiler || exit 1;
	$BUILD src/web/client $type $clean $compiler || exit 1;
	$BUILD src/web/server $type $clean $compiler || exit 1;
fi
cd tests/web;
$BUILD `pwd` $type $clean $compiler || exit 1;

if [ ! -f $JDE_DIR/bin/config/Tests.Web.json ]; then ln -s `pwd`/config/Tests.Web.json $JDE_DIR/bin/config; fi;
exit $?