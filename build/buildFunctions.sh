function buildRelativePath() {
	fileWorkspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/tests
	if [[ $fileWorkspaceFolder == *"jde/Framework/source" ]]; then
		relativePath="/libs/framework";
	elif [[ ${fileWorkspaceFolder##*Public/} != $fileWorkspaceFolder ]]; then
		relativePath=${fileWorkspaceFolder##*Public/};
	elif [[ $fileWorkspaceFolder == *"web/client" ]]; then
		relativePath="/web/client";
	elif [[ $fileWorkspaceFolder == *"web/server" ]]; then
		relativePath="/web/server";
	elif [[ $fileWorkspaceFolder == *"db/src" ]]; then
		relativePath="/db/framework";
	elif [[ $fileWorkspaceFolder == *"drivers/mysql" ]]; then
		relativePath="/db/drivers/mysql";
	elif [[ $fileWorkspaceFolder == *"libs/ql" ]]; then
		relativePath="/ql";
	elif [[ $fileWorkspaceFolder == *"access/src" ]]; then
		relativePath="/access";
	elif [[ $fileWorkspaceFolder == *"app/shared" ]]; then
		relativePath="/app/shared";
	elif [[ $fileWorkspaceFolder == *"app/client" ]]; then
		relativePath="/app/client";
	elif [[ $fileWorkspaceFolder == *"AppServer/source" ]]; then
		relativePath="/lib";
	elif [[ $fileWorkspaceFolder == *"opc/src" ]]; then
		relativePath="/opc/shared";
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
	workspaceFolder=$1; #/home/duffyj/code/jde/Public/libs/web/client
	fileWorkspaceFolder=$2; #/home/duffyj/code/jde/Public/libs/web/tests
	relativeFile=$3; #../tests/mocks/ServerSocketSession.cpp
	buildRoot=$4;  #/mnt/ram/jde/Debug/libs
	buildRelativePath=`buildRelativePath $fileWorkspaceFolder`; #/mnt/ram/jde/Debug/libs/tests
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public/libs/web/tests/mocks/ServerSocketSession.cpp

	echo "workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRoot=$buildRoot, projectName=$projectName, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile";
	cd $buildRoot/$buildRelativePath;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	echo $buildRoot/$buildRelativePath/make $buildFile.o;
	make ${buildFile}.o;
}
