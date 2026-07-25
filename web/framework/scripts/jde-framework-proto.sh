#!/bin/bash
wsDir=${1:-`pwd`};
clean=${2:-0};
baseDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
webDir=$(dirname $(readlink -e $baseDir));
source $JDE_BASH/build/common.sh;
frameworkDir=$baseDir;
source $frameworkDir/scripts/common-proto.sh;
echo jde-framework-proto.sh wsDir=$wsDir clean=$clean;

#npm installs into the current directory, but create()'s pbjs/pbts guard reads $wsDir - the two must be the same
#place or the install never satisfies the guard.  Everything below is workspace-relative as well, so cd once here.
cd $wsDir || { echo `pwd`; echo cd $wsDir failed - not a directory; exit 1; };
if [ ! -d node_modules ]; then echo $wsDir is not a workspace dir - no node_modules.; exit 1; fi;

npm list | grep protobufjs-cli &> /dev/null;
if [ $? -ne 0 ]; then
	echo installing protobufjs-cli;
	npm install protobufjs-cli;
fi;
npm list | grep protobufjs &> /dev/null;
if [ $? -ne 0 ]; then
	echo installing protobufjs
	npm install protobufjs
else
	echo protobufjs already installed
fi;

cd projects/jde-framework/src/lib;
moveToDir proto;

declare -A webFiles;
if [ ! -f Web.FromServer.d.ts ] || [ $clean == 1 ]; then webFiles[Web.FromServer]=web_from_server; fi;
echo 'Creating web proto files';
create $JDE_BASH/libs/web/client/proto webFiles $wsDir;
echo 'Created web proto files';
declare -A appFiles;
if [ ! -f Log.d.ts ] || [ $clean == 1 ]; then appFiles[Log]=log; fi;
if [ ! -f App.FromClient.d.ts ] || [ $clean == 1 ]; then appFiles[App.FromClient]=app_from_client; fi;
if [ ! -f App.FromServer.d.ts ] || [ $clean == 1 ]; then appFiles[App.FromServer]=app_from_server; fi;
if [ ! -f App.d.ts ] || [ $clean == 1 ]; then appFiles[App]=app; fi;
if [ ! -f Common.d.ts ] || [ $clean == 1 ]; then appFiles[Common]=common; fi;
echo 'Creating application proto files';
create $JDE_BASH/libs/app/shared/proto appFiles $wsDir;
echo 'Created application proto files';

echo jde-framework-proto.sh complete.