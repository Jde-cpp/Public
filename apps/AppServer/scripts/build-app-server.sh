#!/bin/bash
branch=${1:-master}
clean=${2:-0};
shouldFetch=${3:-0};
tests=${4:-1}

temp=$(readlink -f "${BASH_SOURCE[0]}"); scriptName=$(basename "$temp"); unset temp;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [ -z $JDE_BASH ]; then JDE_BASH=$scriptDir/jde; fi;
if [ ! -d $JDE_BASH ]; then mkdir jde; fi;
if [ -z $REPO_DIR ]; then REPO_DIR=$scriptDir/libraries; fi;

echo $scriptName clean=$clean shouldFetch=$shouldFetch tests=$tests
if [ ! -d $JDE_BASH/Framework ]; then cd $JDE_BASH; git clone -b $branch --single-branch https://github.com/Jde-cpp/Framework.git; cd ..; fi;

if ! source $JDE_BASH/Framework/scripts/common-error.sh; then exit 1; fi;
source $JDE_BASH/Framework/scripts/source-build.sh;
$JDE_BASH/Framework/scripts/framework-build.sh $branch $clean $shouldFetch $tests;

if windows; then
	echo ---------------ODBC--------------
	cd $JDE_BASH;
	fetchDefault master Odbc 0;
	buildCMake Jde.DB.Odbc 0 DB.Odbc;
fi;

cd $JDE_BASH;
fetchDefault master Google 0;

echo -------------Web.Server------------
cd $JDE_BASH/Public/src/web/server;
buildCMake Jde.Web.Server;
if [ $tests -eq 1 ]; then
	cd $JDE_BASH/Public/tests/web;
	buildCMake Jde.Web.Tests;
fi;

echo -------------App.Shared------------
cd $JDE_BASH/Public/src/app/shared;
buildCMake Jde.App.Shared;

fetchDefault master AppServer 0;
buildCMake Jde.App.ServerLib 0 Jde.App.Server.lib;
cd $JDE_BASH/AppServer;
buildCMake Jde.App.Server 0 Jde.App.Server.exe;

if [ $tests -eq 1 ]; then
	cd $JDE_BASH/AppServer/tests;
	buildCMake Jde.App.Server.Tests 0 Jde.App.Server.Tests.exe;
fi;

echo >&2 '
************sss
*** DONE ***
************
'
# findProtoc;
# cd $JDE_BASH/AppServer/source; moveToDir types; moveToDir proto;
# file=FromServer;
# if [[ ! -f $file.pb.h || $shouldFetch -eq 1 ]]; then
# 	mklink FromServer.proto $JDE_BASH/Public/src/web/proto;
# 	protoc --cpp_out dllexport_decl=JDE_WEB_EXPORTS:. -I. $file.proto;
# 	rm FromServer.pb.cc;
# 	sed -i -e 's/JDE_WEB_EXPORTS/ΓW/g' $file.pb.h;
# 	sed -i '1s/^/\xef\xbb\xbf/' $file.pb.h;
# fi;
# file=AppFromServer;
# if [[ ! -f $file.pb.h || $shouldFetch -eq 1 ]]; then
# 	mklink $file.proto $JDE_BASH/Public/jde/appServer/proto;
# 	protoc --cpp_out=. $file.proto;
# 	sed -i 's/PROTOBUF_CONSTINIT StringValueDefaultTypeInternal/StringValueDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT ApplicationDefaultTypeInternal/ApplicationDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT StatusDefaultTypeInternal/StatusDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT ApplicationStringDefaultTypeInternal/ApplicationStringDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT CustomDefaultTypeInternal/CustomDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT GraphQLDefaultTypeInternal/GraphQLDefaultTypeInternal/' $file.pb.cc;
# fi;
# file=AppFromClient;
# if [[ ! -f $file.pb.h || $shouldFetch -eq 1 ]]; then
# 	mklink $file.proto $JDE_BASH/Public/jde/appServer/proto;
# 	sed -i 's/PROTOBUF_CONSTINIT CustomDefaultTypeInternal/CustomDefaultTypeInternal/' $file.pb.cc;
# 	sed -i 's/PROTOBUF_CONSTINIT GraphQLDefaultTypeInternal/GraphQLDefaultTypeInternal/' $file.pb.cc;
# 	protoc --cpp_out=. $file.proto;
# fi;
# dir=$JDE_BASH/public/src/web/proto
# protoDir=$appServerDir/source/types/proto
# function appProto
# {
# 	file=$1
# 	dest=$protoDir/$file.pb.cc;
# 	if test ! -f $dest; then
# 		createProto $dir $file;
# 		mv $dir/$file.pb.cc $protoDir/$file.pb.cc
# 		cp $dir/$file.pb.h $protoDir/$file.pb.h
# 	fi;
# }

# appProto FromServer
# sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StatusDefaultTypeInternal _Status_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StatusDefaultTypeInternal _Status_default_instance_;/' $protoDir/$file.pb.cc;
# sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ApplicationStringDefaultTypeInternal _ApplicationString_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ApplicationStringDefaultTypeInternal _ApplicationString_default_instance_;/' $protoDir/$file.pb.cc;
# sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ErrorMessageDefaultTypeInternal _ErrorMessage_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ErrorMessageDefaultTypeInternal _ErrorMessage_default_instance_;/' $protoDir/$file.pb.cc;
# sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' $protoDir/$file.pb.cc;
# appProto FromClient;
# echo x=$protoDir/$file.pb.cc;
# sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' $protoDir/$file.pb.cc;


# cd $appServerDir/source;
# build AppServer 0 Jde.AppServer.exe;
# cd $appServerDir/tests;
# build Tests.AppServer 0 Jde.AppServer.exe;
