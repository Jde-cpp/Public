function buildRelativePath() {
	fileWorkspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/tests
	absoluteFile=$2; #/home/duffyj/code/jde/IotWebsocket/source/HttpRequestAwait.cpp
	buildRoot=$3;
	if [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
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
	if [[ $buildRelativePath == *"libs/fwk/lib"* ]]; then
		target=Jde;
	elif [[ $buildRelativePath == *"libs/fwk/tests"* ]]; then
		target=Jde.Fwk.Tests;
	elif [[ $buildRelativePath == *"libs/access/lib"* ]]; then
		target=Jde.Access;
	elif [[ $buildRelativePath == *"libs/access/tests"* ]]; then
		target=Jde.Access.Tests;
	elif [[ $buildRelativePath == *"libs/db/lib"* ]]; then
		target=Jde.DB;
	elif [[ $buildRelativePath == *"libs/db/drivers/mysql"* ]]; then
		target=Jde.DB.MySql;
	elif [[ $buildRelativePath == *"web/client"* ]]; then
		target=Jde.Web.Client;
	elif [[ $buildRelativePath == *"web/server"* ]]; then
		target=Jde.Web.Server;
	elif [[ $buildRelativePath == *"libs/web/tests"* ]]; then
		target=Jde.Web.Tests;
	elif [[ $buildRelativePath == *"app/client" ]]; then
		target=Jde.App.Client;
	elif [[ $buildRelativePath == *"app/shared"* ]]; then
		target=Jde.App.Shared;
	elif [[ $buildRelativePath == "apps/AppServer"* ]]; then
		target=Jde.App.Server;
	elif [[ $buildRelativePath == *"libs/opc/lib"* ]]; then
		target=Jde.Opc;
	elif [[ $buildRelativePath == *"libs/ql"* ]]; then
		target=Jde.QL;
	elif [[ $buildRelativePath == *"OpcGateway/lib"* ]]; then
		target=Jde.Opc.GatewayLib;
	elif [[ $buildRelativePath == "apps/OpcGateway/tests"* ]]; then
		target=Jde.Opc.Tests;
	elif [[ $buildRelativePath == "apps/OpcServer/lib"* ]]; then
		target=Jde.Opc.ServerLib;
	elif [[ $buildRelativePath == "apps/OpcServer/tests"* ]]; then
		target=Jde.Opc.Server.Tests;
	else
		target=foo;
	fi;
	echo "fileDirname:$fileDirname, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, target=$target";
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