#!/bin/bash
if [ ! -d node_modules ]; then echo must run from angular dir; exit 1; fi;
angularDir=`pwd`;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
cd $scriptDir/..;
baseDir=`pwd`;
source $JDE_BASH/build/common.sh;

cd $angularDir;