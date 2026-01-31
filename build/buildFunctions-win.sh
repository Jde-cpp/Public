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
	relativeFile=$1; #/libs/access/tests/main.cpp
	if [[ $relativeFile == *"opc/src"* ]]; then
		project="Jde.Opc";
	elif [[ $relativeFile == *"fwk/src"* ]]; then
		project="Jde";
	elif [[ $relativeFile == *"fwk/tests"* ]]; then
		project="Jde.Framework.Tests";
	elif [[ $relativeFile == *"web/tests" ]]; then
		project="Jde.Web.Tests";
	elif [[ $relativeFile == *"access/src"* ]]; then
		project="Jde.Access";
	elif [[ $relativeFile == *"access/tests"* ]]; then
		project="Jde.Access.Tests";
	elif [[ $relativeFile == *"AppServer/src"* ]]; then
		project="Jde.App.ServerLib";
	fi;
	echo $project.vcxproj;
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
	cd $buildRoot/$buildRelativePath;
	project=`projectName $relativeFile`;
	echo $buildRoot/$buildRelativePath msbuild.exe $project -p:Configuration=Debug
	msbuild.exe $project -p:Configuration=Debug -v:m
}
function compile() {
	source $JDE_BASH/build/common.sh;
	toBashDir $1 workspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $2 fileWorkspaceFolder; #/c/Users/duffyj/source/repos/jde/Public
	toBashDir $3 relativeFile; #/libs/access/tests/main.cpp
	toBashDir $4 buildRoot;  # /z/build/msvc
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/c/Users/duffyj/source/repos/jde/Public/libs/access/tests/main.cpp
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder $absoluteFile $relativeFile`; #libs/web/server
	project=`projectName $relativeFile`;
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile, project=$project";
	cd $buildRoot/$buildRelativePath;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	echo $buildRoot/$buildRelativePath msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile
	msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile //v:m
}