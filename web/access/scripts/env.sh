#!/bin/bash
scriptsDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
appRootDir=$(dirname $(readlink -e $scriptsDir));
webDir=$(dirname $(readlink -e $appRootDir));
frameworkDir=$webDir/framework;
source $JDE_BASH/build/common.sh;
source $frameworkDir/scripts/common-proto.sh;
if ! source $JDE_BASH/build/common-error.sh; then exit 1; fi;