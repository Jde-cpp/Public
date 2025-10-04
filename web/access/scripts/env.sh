#!/bin/bash
scriptsDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
appRootDir=$(dirname $(readlink -e $scriptsDir));
jdeBash=$(dirname $(readlink -e $appRootDir/../..));
webDir=$jdeBash/web;
frameworkDir=$webDir/WebFramework;
source $jdeBash/Public/build/scripts/common-error.sh;
source $jdeBash/Public/build/scripts/common.sh;
source $frameworkDir/scripts/common-proto.sh;