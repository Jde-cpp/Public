function buildRelativePath() {
	fileWorkspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/tests
	absoluteFile=$2; #/home/duffyj/code/jde/IotWebsocket/source/HttpRequestAwait.cpp
	buildRoot=$3;
	if [[ $fileWorkspaceFolder == *"jde/Framework/source" ]]; then
		relativePath="libs/framework/lib";
	elif [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
		relativePath=${fileWorkspaceFolder##*Public/};
		filename=$(basename "$absoluteFile");
		if [[ $filename == "main.cpp" ]]; then
			relativePath=${relativePath/src/exe};
		else
			relativePath=${relativePath/src/lib};
		fi;
	else
		relativePath="";
	fi;
	echo $relativePath;
}
function absoluteFile() {
	workspaceFolder=$1;
	relativeFile=$2;
	absoluteFile=`realpath $workspaceFolder/$relativeFile`; #/home/duffyj/code/jde/Framework/source/io/DiskWatcher.cpp
	echo $absoluteFile;
}
function buildProject() {
	fileDirname=$1;
	buildRoot=$2;

	buildRelativePath=`buildRelativePath $fileDirname`;
	if [[ $buildRelativePath == "apps/OpcGateway/tests" ]]; then
		target=Jde.Opc.Tests;
	elif [[ $buildRelativePath == *"web/server" ]]; then
		target=Jde.Web.Server;
	elif [[ $buildRelativePath == *"app/client" ]]; then
		target=Jde.App.Client;
	elif [[ $buildRelativePath == *"app/shared"* ]]; then
		target=Jde.App.Shared;
	elif [[ $buildRelativePath == *"OpcGateway/lib"* ]]; then
		target=Jde.Opc.GatewayLib;
	elif [[ $buildRelativePath == "apps/AppServer"* ]]; then
		target=Jde.App.Server;
	elif [[ $buildRelativePath == "apps/OpcGateway/tests"* ]]; then
		target=Jde.Opc.Tests;
	else
		target=foo;
	fi;
	echo "fileWorkspaceFolder:$fileWorkspaceFolder, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, target=$target";
	cd $buildRoot;
	cmake --build . -j --target $target;
}
function compile() {
	workspaceFolder=$1; #/home/duffyj/code/jde/IotWebsocket/config
	fileWorkspaceFolder=$2; #/home/duffyj/code/jde/Public/libs/web/server
	relativeFile=$3; #../../Public/libs/web/server/IHttpRequestAwait.cpp
	buildRoot=$4;  # /mnt/ram/jde/Debug
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public/libs/web/server/IHttpRequestAwait.cpp
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder $absoluteFile $buildRoot`; #libs/web/server

	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile";
	cd $buildRoot/$buildRelativePath;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"} #shortest-match prefix removal
	filename=$(basename "$absoluteFile");
	if [[ $filename == "main.cpp" ]]; then
		buildFile=src/main.cpp;
	fi;
	echo $buildRoot/$buildRelativePath/make $buildFile.o;
	make ${buildFile}.o;
}