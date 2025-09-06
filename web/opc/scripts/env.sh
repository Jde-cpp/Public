#!/bin/bash
thisDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
baseDir=$thisDir;
webDir=$(dirname $(readlink -e $baseDir/..));
#jdeBash=$(dirname $(readlink -e $webDir));
frameworkDir=$webDir/framework;
source $JDE_BASH/Framework/scripts/common.sh;
source $frameworkDir/scripts/common-proto.sh;
if ! source $JDE_BASH/Framework/scripts/common-error.sh; then exit 1; fi;