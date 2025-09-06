#!/bin/bash
clean=${1:-0};
baseDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
webDir=$(dirname $(readlink -e $baseDir));
source $JDE_BASH/Framework/scripts/common.sh;
frameworkDir=$baseDir;
source $frameworkDir/scripts/common-proto.sh;

if [ ! -d node_modules ]; then echo run from workspace dir.; exit 1; fi;
npm list | grep protobufjs &> /dev/null;
if [ $? -ne 0 ]; then
	echo installing protobufjs
	npm install protobufjs
else
	echo protobufjs already installed
fi;
npm list | grep protobufjs-cli &> /dev/null;
if [ $? -ne 0 ]; then npm install protobufjs-cli; fi;

cd projects/jde-framework/src/lib;
moveToDir proto;
# declare -A commonFiles;
# if [ ! -f FromServer.d.ts ] || [ $clean == 1 ]; then commonFiles[FromServer]=from_server_root; fi;
# create $JDE_BASH/Public/src/web/proto commonFiles;
# echo 'Created common proto files';

declare -A webFiles;
if [ ! -f Web.FromServer.d.ts ] || [ $clean == 1 ]; then webFiles[Web.FromServer]=web_from_server; fi;
echo 'Creating web proto files';
create $JDE_BASH/Public/libs/web/client/proto webFiles;
echo 'Created web proto files';
declare -A appFiles;
if [ ! -f App.FromClient.d.ts ] || [ $clean == 1 ]; then appFiles[App.FromClient]=app_from_client; fi;
if [ ! -f App.FromServer.d.ts ] || [ $clean == 1 ]; then appFiles[App.FromServer]=app_from_server; fi;
if [ ! -f App.d.ts ] || [ $clean == 1 ]; then appFiles[App]=app; fi;
if [ ! -f Common.d.ts ] || [ $clean == 1 ]; then appFiles[Common]=common; fi;
echo 'Creating application proto files';
create $JDE_BASH/Public/libs/app/shared/proto appFiles;
echo 'Created application proto files';

echo jde-framework-proto.sh complete.