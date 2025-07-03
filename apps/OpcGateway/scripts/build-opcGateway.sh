#!/bin/bash
branch=${1:-master}
clean=${2:-0};
shouldFetch=${3:-0};
tests=${4:-1}
target=${5:Debug}
compiler=${6:-msvc}

cmakeTarget="${target^}";
[[ "$branch" = master ]] && branch2=main || branch2="$branch"
echo build-iotwebsocket.sh branch: $branch2 clean: $clean shouldFetch: $shouldFetch tests: $tests;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [ -z $JDE_BASH ]; then export JDE_BASH=$scriptDir/jde; fi;
if [ -z $REPO_DIR ]; then export REPO_DIR=$scriptDir/libraries; fi;
if [ -z $JDE_BUILD_DIR ]; then export $scriptDir/build; fi;

if [ ! -d $JDE_BASH ]; then mkdir jde; fi;
cd jde;
if [ ! -d Public ]; then git clone -b $branch --single-branch https://github.com/Jde-cpp/Public.git; fi;
source Public/build/common.sh;
toBashDir $REPO_DIR REPO_BASH;
cd $JDE_BASH/Public/build;

./buildBoost.sh $target $compiler; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;

#build external libraries
if [ -z $JDE_BUILD_DIR ]; then export JDE_BUILD_DIR=$scriptDir/build; fi;
toBashDir $JDE_BUILD_DIR buildBashDir;
echo buildBashDir: $buildBashDir;

cd $buildBashDir;
mkdir -p $compiler; cd $compiler;
# if (( $clean == 1 )) || [ ! -d libs ]; then
# 	echo ----------------------Build Libs----------------------
# 	exit 0;
# 	rm -f CMakeCache.txt;
# 	cd libs;
# 	echo cmake /c/Users/duffyj/source/repos/jde/Public/build --preset win-msvc;
# 	cmake $JDE_BASH/Public/build --preset win-msvc; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
# 	cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
# fi

compilerDir=$buildBashDir/$compiler;
moveToDir jde;
if [ $tests -eq 1 ]; then
	moveToDir libs;
	jdeLibsDir=`pwd`;
	if (( $clean == 1 )) || [ ! -d crypto ]; then
		moveToDir crypto;
		echo ---------------------Build Crypto----------------------
		#rm -f CMakeCache.txt;
		#cmake $JDE_BASH/Public/libs/crypto --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
		#cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
	fi
	cd $jdeLibsDir;
	if (( $clean == 1 )) || [ ! -d access ]; then
		moveToDir access;
		echo ---------------------Build Access----------------------
		#rm -f CMakeCache.txt;
		#cmake $JDE_BASH/Public/libs/access --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
		#cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
	fi
	cd $jdeLibsDir;
	if (( $clean == 1 )) || [ ! -d web ]; then
		moveToDir web;
		echo -------------------------Build Web--------------------------
		#rm -f CMakeCache.txt;
		#cmake $JDE_BASH/Public/libs/web --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
		#cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
	fi
	cd $jdeLibsDir;
	if (( $clean == 1 )) || [ ! -d opc ]; then
		moveToDir opc;
		echo -------------------------Build Opc--------------------------
		#rm -f CMakeCache.txt;
		#cmake $JDE_BASH/Public/libs/opc --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
		#cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
	fi
fi

#git clone -b $branch --single-branch https://github.com/Jde-cpp/AppServer.git;
cd $compilerDir/jde;
moveToDir apps;
appsDir=`pwd`;
if (( $clean == 1 )) || [ ! -d $JDE_BASH/AppServer ]; then
	echo -------------------------AppServer--------------------------
	cd AppServer;
	rm -f CMakeCache.txt;
#	cmake $JDE_BASH/AppServer --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
#	cmake --build . --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
fi

# if [ ! -d $JDE_BASH/IotWebsocket ]; then
# 	cd $JDE_BASH;
# 	git clone -b $branch --single-branch https://github.com/Jde-cpp/IotWebsocket.git;
# fi
cd $appsDir;
if (( $clean == 1 )) || [ ! -d IotWebsocket ]; then
	moveToDir IotWebsocket;
	echo ------------------------IotWebsocket------------------------
	rm -f CMakeCache.txt;
	cmake $JDE_BASH/IotWebsocket/source --preset win-msvc-jde; if [ $? -ne 0 ]; then echo "cmake Failed"; echo `pwd`; exit 1; fi;
	cmake --build .;  --config $cmakeTarget; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
fi

echo --------------------------Complete--------------------------
exit 0;


/mnt/ram/jde/Debug
cls;rm CMakeCache.txt;cmake /home/duffyj/code/jde/Public/build --preset linux-relWithDebInfo;
cls;rm CMakeCache.txt;cmake /home/duffyj/code/jde/Public/build --preset linux-debug;

cmake



if [ ! -d Framework ]; then git clone -b $branch --single-branch https://github.com/Jde-cpp/Framework.git; fi;
if ! source $JDE_BASH/Framework/scripts/common-error.sh; then exit 1; fi;
#source $JDE_BASH/Framework/scripts/source-build.sh;

$JDE_BASH/Framework/scripts/framework-build.sh $branch $clean $shouldFetch $tests;

if [ ! windows -a ! -z "$install" ]; then #https://www.open62541.org/doc/open62541-1.3.pdf
	sudo apt-get install git build-essential gcc pkg-config cmake python
	sudo apt-get install cmake-curses-gui # for the ccmake graphical interface
	sudo apt-get install libmbedtls-dev # for encryption support
	sudo apt-get install check libsubunit-dev # for unit tests
	sudo apt-get install python-sphinx graphviz # for documentation generation
	sudo apt-get install python-sphinx-rtd-theme # documentation style
fi;

echo -------------Crypto------------
cd $JDE_BASH/Public/src/crypto;
buildCMake Jde.Crypto;
if [ $tests -eq 1 ]; then
	cd $JDE_BASH/Public/tests/crypto;
	buildCMake Jde.Crypto.Tests;
fi;

echo -------------Web.Client------------
cd $JDE_BASH/Public/src/web/client;
buildCMake Jde.Web.Client;

echo -------------App.Shared------------
cd $JDE_BASH/Public/src/app/shared;
buildCMake Jde.App.Shared;

echo -------------App.Client------------
cd $JDE_BASH/Public/src/app/client;
buildCMake Jde.App.Client;

echo -------------Web.Server------------
cd $JDE_BASH/Public/src/web/server;
buildCMake Jde.Web.Server;
if [ $tests -eq 1 ]; then
	cd $JDE_BASH/Public/tests/web;
	buildCMake Jde.Web.Tests;
fi;

echo -------------open62541-------------
cd $REPO_DIR;
if [ ! -d open62541 ]; then git clone https://github.com/Jde-cpp/open62541.git; fi;
cd open62541;
if [ $shouldFetch -eq 1 ]; then
	git pull > /dev/null;
fi;
if windows; then
	export CMAKE_INSTALL_PREFIX=$REPO_DIR/installed;
else
	export CMAKE_INSTALL_PREFIX=$REPO_DIR/install/CXX/$CMAKE_BUILD_TYPE;
fi;
buildCMake open62541;
cd $REPO_DIR/open62541/.build;
if windows; then
	cmake.exe -DBUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX/debug -P cmake_install.cmake;
else
	cmake.exe -DBUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX/asan -P cmake_install.cmake;
fi;
cmake.exe -DBUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX/RelWithDebInfo -P cmake_install.cmake;

echo ----------------iot----------------
cd $JDE_BASH/Public/src/iot;
buildCMake Jde.Iot;
if [ $tests -eq 1 ]; then
	cd $JDE_BASH/Public/tests/iot;
	buildCMake Jde.Iot.Tests;
fi;

if windows; then
	cd $JDE_BASH;
	fetchDefault master Odbc 0;
	buildCMake Jde.DB.Odbc 0 DB.Odbc;
fi;

cd $JDE_BASH;
fetchDefault main IotWebsocket 0;
buildCMake Jde.IotWebsocket 0 Jde.IotWebsocket.exe;

cd $JDE_BASH;moveToDir web;
fetch main IotSite;
./scripts/setup.sh

exit 0;