#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}

frameworkDir=$JDE_DIR/Framework;
source $frameworkDir/scripts/common-error.sh;
makeScript=$frameworkDir/cmake/buildc.sh;
if [ $all -eq 1 ]; then
	echo all eq 1
	$makeScript $frameworkDir/source $type $clean || exit 1;
	echo all=success;
fi
#echo $makeScript `pwd` $type $clean
$makeScript `pwd` $type $clean

exit $?