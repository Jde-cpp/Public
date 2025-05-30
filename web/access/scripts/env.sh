#!/bin/bash
scriptsDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
appRootDir=$(dirname $(readlink -e $scriptsDir));
jdeBash=$(dirname $(readlink -e $appRootDir/../..));
webDir=$jdeBash/web;
frameworkDir=$webDir/WebFramework;
source $jdeBash/Framework/scripts/common-error.sh;
source $jdeBash/Framework/scripts/common.sh;
source $frameworkDir/scripts/common-proto.sh;
#echo scriptsDir=$scriptsDir;
#echo appRootDir=$appRootDir;
#echo webDir=$webDir;
#echo jdeBash=$jdeBash;
#echo frameworkDir=$frameworkDir;