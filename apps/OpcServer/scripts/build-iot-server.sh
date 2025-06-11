#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};
tests=${3:-1}

echo build-iotwebsocket.sh clean=$clean shouldFetch=$shouldFetch tests=$tests;

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
#if [ -z $JDE_DIR ]; then JDE_DIR=$scriptDir/jde; fi;
if [ -z $JDE_BASH ]; then JDE_BASH=$scriptDir/jde; fi;
if [ -z $REPO_DIR ]; then REPO_DIR=$scriptDir/libraries; fi;

if ! source $JDE_BASH/Framework/scripts/common-error.sh; then exit 1; fi;
source $JDE_BASH/Framework/source-build.sh;

$JDE_BASH/Framework/framework-build.sh $clean $shouldFetch $tests;

if [ ! windows -a ! -z "$install" ]; then #https://www.open62541.org/doc/open62541-1.3.pdf
	sudo apt-get install git build-essential gcc pkg-config cmake python
	sudo apt-get install cmake-curses-gui # for the ccmake graphical interface
	sudo apt-get install libmbedtls-dev # for encryption support
	sudo apt-get install check libsubunit-dev # for unit tests
	sudo apt-get install python-sphinx graphviz # for documentation generation
	sudo apt-get install python-sphinx-rtd-theme # documentation style
fi;
echo -------------Placeholder3-------------
cd $REPO_DIR;
if [ ! -d open62541 ]; then git clone https://github.com/Jde-cpp/open62541.git; fi;
cd open62541;
if [ $shouldFetch -eq 1 ]; then
	git pull > /dev/null;
	if [ -f bin/Debug/open62541.lib ]; then rm bin/Debug/open62541.lib; fi;
	if [ -f bin/Release/open62541.lib ]; then rm bin/Debug/open62541.lib; fi;
fi;
if [ ! -f open62541.sln ]; then
	echo open62541.sln - `pwd`
	moveToDir build;
	cmake -DUA_THREADSAFE=100 -DUA_LOGLEVEL=100 ..;
fi;
stage=$JDE_BASH/Public/stage;
if [[ ! -f $stage/debug/open62541.lib || ! -f $stage/release/open62541.lib ]]; then
	build open62541 0;
	cd $stage/debug; mklink open62541.lib $REPO_BASH/open62541/build/bin/Debug;
	cd $stage/release; mklink open62541.lib $REPO_BASH/open62541/build/bin/Release;
fi;

cd $JDE_BASH;
fetchDefault IotWebsocket;
findProtoc;
cd $JDE_BASH/IotWebsocket/source/types/proto;
echo -------------------proto-------------------------;
file=IotFromServer;
if [[ ! -f $file.pb.h || $shouldFetch -eq 1 ]]; then
	protoc --cpp_out=. $file.proto;
	sed -i -e 's/PROTOBUF_CONSTINIT CustomDefaultTypeInternal/CustomDefaultTypeInternal/g' $file.pb.cc;
	sed -i -e 's/PROTOBUF_CONSTINIT QueryResultDefaultTypeInternal/QueryResultDefaultTypeInternal/g' $file.pb.cc;
	sed -i -e 's/PROTOBUF_CONSTINIT StatusDefaultTypeInternal/StatusDefaultTypeInternal/g' $file.pb.cc;
	sed -i -e 's/PROTOBUF_CONSTINIT ApplicationStringDefaultTypeInternal/ApplicationStringDefaultTypeInternal/g' $file.pb.cc;
	sed -i -e 's/PROTOBUF_CONSTINIT ErrorDefaultTypeInternal/ErrorDefaultTypeInternal/g' $file.pb.cc;
fi;
file=IotFromClient;
if [[ ! -f $file.pb.h  || $shouldFetch -eq 1 ]]; then
	protoc --cpp_out=. $file.proto;
	sed -i -e 's/PROTOBUF_CONSTINIT GraphQLDefaultTypeInternal/GraphQLDefaultTypeInternal/g' $file.pb.cc;
	sed -i -e 's/PROTOBUF_CONSTINIT CustomDefaultTypeInternal/CustomDefaultTypeInternal/g' $file.pb.cc;
fi;
cd ../..;

build IotWebsocket 0 Jde.IotWebsocket.exe;

cd $JDE_BASH;moveToDir web;
fetch IotSite;
