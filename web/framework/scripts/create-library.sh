#!/bin/bash
library=${1};  #jde-blockly
dir=${2}; #/home/user/.../WebBlockly
overwrite=${3:-0}; #true
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
echo starting Library $library;
source $JDE_BASH/Framework/scripts/common.sh;
controlDir=$dir/control;
echo controlDir=$controlDir;
echo pwd=`pwd`;
if [ $overwrite != 0 ]; then
	rm -rf projects/$library;
fi;
if [ ! -d projects/$library ]; then
	configPath=.compilerOptions.paths.\"$library\"
	angularConfig=`jq .projects.\"$library\".root angular.json`;
	if [ ! -z "$angularConfig" ]; then
		echo removing from [angular][tsconfig].json;
		if [ \"$(head -c 2 tsconfig.json)\" == \"/*\" ]; then
			sed -i '1d' tsconfig.json;
			echo removed comment;
		fi;
		if [ \"$(head -c 2 tsconfig.json)\" == \"/*\" ]; then
			sed -i '1d' tsconfig.json;
			echo removed comment;
		fi;		cmd="del(.projects.\"$library\")"
		jq $cmd angular.json > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq $cmd angular.json; exit 1; fi;
		mv temp.json angular.json;
		cmd="del($configPath)";
		jq $cmd tsconfig.json > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq $cmd tsconfig.json; exit 1; fi;
		mv temp.json tsconfig.json;
	fi;

	ng analytics off;#should add to angular.json "cli": {"analytics": false},
	ng g library $library --defaults; if [ $? -ne 0 ]; then echo `pwd`; echo ng g library $library --defaults; exit 1; fi;
	configConent="[\"./projects/$library/src/public-api\", \"./dist/$library/$library\", \"./dist/$library\"]";
	jq "$configPath = $configConent" tsconfig.json > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq \"$configPath = $configConent\" tsconfig.json; exit 1; fi;
	mv temp.json tsconfig.json;
fi;
cd projects/$library;
#if [ -f $controlDir/package.json ]; then addHard package.json $controlDir; fi;
#if [ -f $controlDir/tsconfig.lib.json ]; then addHard tsconfig.lib.json $controlDir; fi;
#if [ -d $controlDir/environments ]; then addHardDir environments $controlDir; fi;
#if [ -d $controlDir/styles ]; then addHardDir styles $controlDir; fi;
cd src;
addHard public-api.ts $controlDir/src;
cd lib;
if [ -f $library.component.ts ]; then rm $library.component.*; fi;
if [ -f $library.service.ts ]; then rm $library.service.*; fi;