#!/bin/bash
source ${BASH_SOURCE%/*}/buildFunctions.sh; #shared helpers: buildRelativePath (src->lib/exe mapping), absoluteFile. Function bodies are subshells - see note there.

#vcxproj/ninja target name for a source file - msbuild and single-file compiles need it; buildProject's ninja branch uses <dir>/all instead.
#Echoes empty when unmapped - callers must check, or ninja would silently build everything.
function projectName() (
	absoluteFile=$1;
	filename=$(basename "$absoluteFile");
	project="";
	if [[ $filename == "main.cpp" ]]; then
		if [[ $absoluteFile == *"AppServer"* ]]; then project="Jde.App.Server";
		elif [[ $absoluteFile == *"OpcGateway"* ]]; then project="Jde.Opc.Gateway";
		elif [[ $absoluteFile == *"OpcServer"* ]]; then project="Jde.Opc.Server";
		fi;
	elif [[ $absoluteFile == *"fwk/src"* ]]; then project="Jde";
	elif [[ $absoluteFile == *"fwk/tests"* ]]; then project="Jde.Fwk.Tests";
	elif [[ $absoluteFile == *"access/tests"* ]]; then project="Jde.Access.Tests";
	elif [[ $absoluteFile == *"config/sql/sqlite"* ]]; then #native-proc modules
		if [[ $absoluteFile == *"AppServer"* || $absoluteFile == *"access"* ]]; then project="Jde.DB.Sqlite.AppServer"; #access procs are registered in AppServer's module
		elif [[ $absoluteFile == *"OpcGateway"* ]]; then project="Jde.DB.Sqlite.OpcGateway";
		elif [[ $absoluteFile == *"OpcServer"* ]]; then project="Jde.DB.Sqlite.OpcServer";
		fi;
	elif [[ $absoluteFile == *"access/src"* ]]; then project="Jde.Access";
	elif [[ $absoluteFile == *"db/drivers/sqlite/tests"* ]]; then project="Jde.DB.Sqlite.Tests";
	elif [[ $absoluteFile == *"db/drivers/sqlite"* ]]; then project="Jde.DB.Sqlite";
	elif [[ $absoluteFile == *"db/drivers/mysql"* ]]; then project="Jde.DB.MySql";
	elif [[ $absoluteFile == *"db/drivers/odbc"* ]]; then project="Jde.DB.Odbc";
	elif [[ $absoluteFile == *"db/src"* ]]; then project="Jde.DB";
	elif [[ $absoluteFile == *"libs/ql"* ]]; then project="Jde.QL";
	elif [[ $absoluteFile == *"web/client"* ]]; then project="Jde.Web.Client";
	elif [[ $absoluteFile == *"web/server"* ]]; then project="Jde.Web.Server";
	elif [[ $absoluteFile == *"web/tests"* ]]; then project="Jde.Web.Tests";
	elif [[ $absoluteFile == *"app/shared"* ]]; then project="Jde.App.Shared";
	elif [[ $absoluteFile == *"app/client"* ]]; then project="Jde.App.Client";
	elif [[ $absoluteFile == *"opc/src"* ]]; then project="Jde.Opc";
	elif [[ $absoluteFile == *"AppServer/src"* ]]; then project="Jde.App.ServerLib";
	elif [[ $absoluteFile == *"OpcGateway/src"* ]]; then project="Jde.Opc.GatewayLib";
	elif [[ $absoluteFile == *"OpcGateway/tests"* ]]; then project="Jde.Opc.Tests";
	elif [[ $absoluteFile == *"OpcServer/src"* ]]; then project="Jde.Opc.ServerLib";
	elif [[ $absoluteFile == *"OpcServer/tests"* ]]; then project="Jde.Opc.Server.Tests";
	fi;
	echo $project;
)
function buildProject() (
	source $JDE_BASH/build/common.sh; #toBashDir
	toBashDir $1 workspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $2 fileWorkspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $3 relativeFile; #libs\access\tests\AuthTests.cpp
	buildRoot=$4;
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	buildRelativePath=`buildRelativePath $workspaceFolder $absoluteFile`; #libs/web/server
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, absoluteFile=$absoluteFile, buildRelativePath=$buildRelativePath";
	if [ -f "$buildRoot/build.ninja" ]; then
		log=$buildRoot/${buildRelativePath//\//.}.output;
		echo $log | tee $log;
		set -o pipefail;
		echo ninja -C $buildRoot $buildRelativePath/all | tee -a $log; #CMake's Ninja generator makes a phony <dir>/all per directory - no target map needed.
		ninja -C $buildRoot $buildRelativePath/all | tee -a $log;
	else
		project=`projectName $absoluteFile`;
		if [[ -z $project ]]; then echo "buildProject: no project mapping for $absoluteFile"; return 1; fi;
		cd $buildRoot/$buildRelativePath || return 1;
		echo $buildRoot/$buildRelativePath msbuild.exe $project.vcxproj -p:Configuration=Debug;
		msbuild.exe $project.vcxproj -p:Configuration=Debug -v:m;
	fi
)
function compile() (
	source $JDE_BASH/build/common.sh; #toBashDir
	toBashDir $1 workspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $2 fileWorkspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $3 relativeFile; #/libs/access/tests/main.cpp
	toBashDir $4 buildRoot;  # /z/build/msvc
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	buildRelativePath=`buildRelativePath $workspaceFolder $absoluteFile`; #libs/web/server
	project=`projectName $absoluteFile`;
	if [[ -z $project ]]; then echo "compile: no project mapping for $absoluteFile"; return 1; fi;
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile, project=$project";
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"};
	if [ -f "$buildRoot/build.ninja" ]; then
		log=$project.output;
		echo `pwd`/$log | tee $log;
		set -o pipefail;
		echo ninja -C $buildRoot ${buildRelativePath}/CMakeFiles/$project.dir/${buildFile}.obj | tee -a $log;
		ninja -C $buildRoot ${buildRelativePath}/CMakeFiles/$project.dir/${buildFile}.obj | tee -a $log;
	else
		cd $buildRoot/$buildRelativePath || return 1;
		echo $buildRoot/$buildRelativePath msbuild.exe $project.vcxproj -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile;
		msbuild.exe $project.vcxproj -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile //v:m; #single-file ClCompile via property is IDE-flavored; outside VS it may compile the whole project.
	fi
)
function reconfig() (
	buildRoot=$1;
	debugPreset=$2;
	sourceDir=$3;
	mkdir -p $buildRoot;
	cd $buildRoot || return 1;
	rm -f CMakeCache.txt;
	echo `pwd` > cmake.output;
	echo "cmake $sourceDir -B $buildRoot -Wno-dev --preset $debugPreset 2>&1" | tee -a cmake.output;
	cmake "$sourceDir" -B "$buildRoot" -Wno-dev --preset "$debugPreset" 2>&1 | tee -a cmake.output; #-B: only win-clang-debug defines binaryDir, so msvc presets would configure in-source under CMake 4.x. $sourceDir (not $JDE_DIR) so Public2 checkouts work.
	if [ -f "$buildRoot/compile_commands.json" ]; then
		mv $buildRoot/compile_commands.json $sourceDir/compile_commands.json;
	fi
)
