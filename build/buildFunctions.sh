function buildRelativePath() {
	fileWorkspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/tests
	absoluteFile=$2; #/home/duffyj/code/jde/IotWebsocket/source/HttpRequestAwait.cpp
	buildRoot=$3;
	if [[ $fileWorkspaceFolder == *"jde/Framework/source" ]]; then
		relativePath="/libs/framework";
	elif [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
		relativePath=${fileWorkspaceFolder##*Public/};
		#if [[ $absoluteFile == *"apps"* ]]; then
		#	relativePath="";
		#elif [[ $relativePath == *"src" ]]; then
		#	if [[ $buildRoot == *"apps"* ]]; then
		#		relativePath="../../libs";
		#	else
				relativePath=${relativePath/src/lib};
		#	fi;
		#fi;
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
function absoluteFile() {
	workspaceFolder=$1;
	relativeFile=$2;
	absoluteFile=`realpath $workspaceFolder/$relativeFile`; #/home/duffyj/code/jde/Framework/source/io/DiskWatcher.cpp
	echo $absoluteFile;
}
function buildProject() {
	fileWorkspaceFolder=$1;
	buildRoot=$2;
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder`;
	echo "fileWorkspaceFolder:$fileWorkspaceFolder, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath";
	cd $buildRoot/$buildRelativePath;
	make -j;
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
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	echo $buildRoot/$buildRelativePath/make $buildFile.o;
	make ${buildFile}.o;
}