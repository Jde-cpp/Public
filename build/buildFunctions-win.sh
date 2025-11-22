function buildRelativePath() {
	fileWorkspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/tests
	absoluteFile=$2; #/home/duffyj/code/jde/IotWebsocket/source/HttpRequestAwait.cpp
	if [[ $fileWorkspaceFolder == *"jde/Framework/source" ]]; then
		relativePath="/libs/framework";
	elif [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
		relativePath=${fileWorkspaceFolder##*Public/};
		if [[ $relativePath == *"src" ]]; then
			relativePath=${relativePath/src/lib};
		fi;
	elif [[ $absoluteFile == *"IotWebsocket/source"* ]]; then
		relativePath="/apps/OpcClient";
	elif [[ $fileWorkspaceFolder == *"AppServer/source" ]]; then
		relativePath="/apps/AppServer/lib";
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
	# elif [[ $fileWorkspaceFolder == *"access/src" ]]; then
	# 	relativePath="/access";
	# elif [[ $fileWorkspaceFolder == *"app/shared" ]]; then
	# 	relativePath="/app/shared";
	# elif [[ $fileWorkspaceFolder == *"app/client" ]]; then
	# 	relativePath="/app/client";
	else
		relativePath="";
	fi;
	echo $relativePath;
}
function projectName() {
	fileWorkspaceFolder=$1;
	if [[ $fileWorkspaceFolder == *"opc/src"* ]]; then
		project="Jde.Opc.vcxproj";
	elif [[ $fileWorkspaceFolder == *"fwk/src"* ]]; then
		project="Jde.vcxproj";
	elif [[ $fileWorkspaceFolder == *"fwk/tests"* ]]; then
		project="Jde.Framework.Tests.vcxproj";
	elif [[ $fileWorkspaceFolder == *"web/tests" ]]; then
		project="Jde.Web.Tests.vcxproj";
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
	toBashDir $1 fileWorkspaceFolder;
	buildRoot=$2;
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder`;
	echo "fileWorkspaceFolder:$fileWorkspaceFolder, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath";
	cd $buildRoot/$buildRelativePath;
	project=`projectName $fileWorkspaceFolder`;
	echo $buildRoot/$buildRelativePath msbuild.exe $project -p:Configuration=Debug
	msbuild.exe $project -p:Configuration=Debug //v:m
}
function compile() {
	source $JDE_BASH/build/common.sh;
	toBashDir $1 workspaceFolder; #/home/duffyj/code/jde/IotWebsocket/config
	toBashDir $2 fileWorkspaceFolder; #/home/duffyj/code/jde/Public/libs/web/server
	toBashDir $3 relativeFile; #../../Public/libs/web/server/IHttpRequestAwait.cpp
	toBashDir $4 buildRoot;  # /mnt/ram/jde/Debug
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public/libs/web/server/IHttpRequestAwait.cpp
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder $absoluteFile`; #libs/web/server
	project=`projectName $fileWorkspaceFolder`;
	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile, project=$project";
	cd $buildRoot/$buildRelativePath;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	echo $buildRoot/$buildRelativePath msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile
	msbuild.exe $project -p:Configuration=Debug -t:ClCompile -p:ClCompile=$relativeFile //v:m
}