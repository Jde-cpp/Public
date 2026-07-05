function buildRelativePath() {
	fileWorkspaceFolder=$1; #/c/Users/duffyj/source/repos/jde/Public
	absoluteFile=$2; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	relativeFile=$3; #/libs/access/tests/main.cpp
	if [[ $fileWorkspaceFolder == *"jde/Framework/src"* ]]; then
		relativePath="libs/framework";
	elif [[ $relativeFile == *"access/src"* ]]; then
		relativePath="libs/access/lib";
	elif [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
		relativePath=${fileWorkspaceFolder##*Public/};
		if [[ $relativePath == *"src" ]]; then
			relativePath=${relativePath/src/lib};
		fi;
	elif [[ $absoluteFile == *"IotWebsocket/src"* ]]; then
		relativePath="apps/OpcClient";
	elif [[ $relativeFile == *"AppServer/src"* ]]; then
		relativePath="apps/AppServer/lib";
	# elif [[ $fileWorkspaceFolder == *"web/client" ]]; then
	# 	relativePath="/web/client";
	# elif [[ $fileWorkspaceFolder == *"web/server" ]]; then
	# 	relativePath="/web/server";
	# elif [[ $fileWorkspaceFolder == *"db/src" ]]; then
	# 	relativePath="/db/framework";
	# elif [[ $fileWorkspaceFolder == *"drivers/mysql" ]]; then
	# 	relativePath="/db/drivers/mysql";
	# elif [[ $fileWorkspaceFolder == *"libs/ql" ]]; then
	# 	relativePath="/ql";
	# elif [[ $fileWorkspaceFolder == *"app/shared" ]]; then
	# 	relativePath="/app/shared";
	# elif [[ $fileWorkspaceFolder == *"app/client" ]]; then
	# 	relativePath="/app/client";
	else
		relativePath=$(dirname "$relativeFile");
		relativePath=${relativePath:1}; #remove leading /
	fi;

	echo $relativePath;
}
function projectName() {
	#echo "projectName called with $1";
	absoluteFile=$1; #/libs/access/tests/main.cpp
	if [[ $absoluteFile == *"opc/src"* ]]; then
		project="Jde.Opc";
	elif [[ $absoluteFile == *"fwk/src"* ]]; then
		project="Jde";
	elif [[ $absoluteFile == *"fwk/tests"* ]]; then
		project="Jde.Framework.Tests";
	elif [[ $absoluteFile == *"web/client"* ]]; then
		project="Jde.Web.Client";
	elif [[ $absoluteFile == *"web/tests"* ]]; then
		project="Jde.Web.Tests";
	elif [[ $absoluteFile == *"access/src"* ]]; then
		project="Jde.Access";
	elif [[ $absoluteFile == *"access/tests"* ]]; then
		project="Jde.Access.Tests";
	elif [[ $absoluteFile == *"AppServer/src"* ]]; then
		project="Jde.App.ServerLib";
	elif [[ $absoluteFile == *"db/src"* ]]; then
		project="Jde.DB";
	elif [[ $absoluteFile == *"OpcGateway/src"* ]]; then
		project="Jde.Opc.GatewayLib";
	elif [[ $absoluteFile == *"/OpcGateway/tests"* ]]; then
		project="Jde.Opc.Tests";
	elif [[ $absoluteFile == *"OpcServer/tests"* ]]; then
		project="Jde.Opc.Server.Tests";
	fi;
	echo $project;
}
function absoluteFile() {
	workspaceFolder=$1;
	relativeFile=$2;
	absoluteFile=`realpath $workspaceFolder/$relativeFile`; #/home/duffyj/code/jde/Framework/source/io/DiskWatcher.cpp
	echo $absoluteFile;
}
function buildProject() {
	source $JDE_BASH/build/common.sh;
	toBashDir $1 workspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $2 fileWorkspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $3 relativeFile; #libs\access\tests\AuthTests.cpp
	buildRoot=$4;
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder $absoluteFile $relativeFile`; #libs/web/server
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, absoluteFile=$absoluteFile, buildRelativePath=$buildRelativePath";
	project=`projectName $absoluteFile`;
	if [ -f "$buildRoot/build.ninja" ]; then
		log=$buildRoot/$project.output;
		echo $log | tee $log;
		echo `pwd` | tee -a $log;
		echo ninja -C $buildRoot ${project}  | tee -a $log;
		set -o pipefail;
		ninja -C $buildRoot ${project} | tee -a $log;
	else
		cd $buildRoot/$buildRelativePath;
		echo $buildRoot/$buildRelativePath msbuild.exe $project.vcxproj -p:Configuration=Debug
		msbuild.exe $project.vcxproj -p:Configuration=Debug -v:m
	fi
}
function compile() {
	source $JDE_BASH/build/common.sh;
	toBashDir $1 workspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $2 fileWorkspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $3 relativeFile; #/libs/access/tests/main.cpp
	toBashDir $4 buildRoot;  # /z/build/msvc
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder $absoluteFile $relativeFile`; #libs/web/server
	project=`projectName $absoluteFile`;
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile, project=$project";
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	if [ -f "$buildRoot/build.ninja" ]; then
		log=$project.output;
		echo `pwd`/$log | tee $log;
		echo ninja -C $buildRoot ${buildRelativePath}/CMakeFiles/$project.dir/${buildFile}.obj | tee -a $log;
		set -o pipefail;
		ninja -C $buildRoot ${buildRelativePath}/CMakeFiles/$project.dir/${buildFile}.obj | tee -a $log;
	else
		cd $buildRoot/$buildRelativePath;
		echo $buildRoot/$buildRelativePath msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile
		msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile //v:m
	fi
}
function reconfig() {
	buildRoot=$1;
	debugPreset=$2;
	sourceDir=$3;
	mkdir -p $buildRoot;
	cd $buildRoot;
	rm -f CMakeCache.txt;
	tput reset;
	echo `pwd` > cmake.output;
	echo "cmake $JDE_DIR -Wno-dev --preset $debugPreset 2>&1" | tee -a cmake.output;
	cmake "$JDE_DIR" -Wno-dev --preset "$debugPreset" 2>&1 | tee -a cmake.output;
	if [ -f "$buildRoot/compile_commands.json" ]; then
		mv $buildRoot/compile_commands.json $sourceDir/compile_commands.json;
	fi
}
function build() {
	buildRoot=$1;
	target=$2;
	cd $buildRoot;
	set -o pipefail;
	tput reset;
	echo `pwd`/$target.output > $target.output;
	echo "cmake --build . -j --target $target" | tee -a $target.output;
	cmake --build . -j --target $target | tee -a $target.output;
}
function buildTests() {
	baseProject=$2;
	if [ $baseProject == "Jde.Opc.Gateway" ]; then
		baseProject="Jde.Opc";
	fi;
	build $1 $baseProject.Tests;
}
