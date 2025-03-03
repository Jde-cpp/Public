function buildDir() {
	fileWorkspaceFolder=$1;#/home/duffyj/code/jde/Framework/source
	buildRoot=$2;#/mnt/ram/jde/Public/libs/web/Debug
	if [[ $fileWorkspaceFolder == *"jde/Framework/source" ]]; then
		relativePath="/framework";
	elif [[ $fileWorkspaceFolder == *"tests" ]]; then
		relativePath="/tests";
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
	echo "$buildRoot$relativePath";
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
	buildDir=`buildDir $fileWorkspaceFolder $buildRoot`;
	cd $buildDir;
	make -j;
}
function compile() {
	workspaceFolder=$1
	fileWorkspaceFolder=$2;
	relativeFile=$3;
	buildRoot=$4;
	#echo workspaceFolder=$workspaceFolder;
	#echo fileWorkspaceFolder=$fileWorkspaceFolder;
	#echo relativeFile=$relativeFile;

	#fileBasename=$(basename $relativeFile);
	#echo fileBasename=$fileBasename;
	buildDir=`buildDir $fileWorkspaceFolder $buildRoot`;
	cd $buildDir;
	#echo buildDir=`pwd`;
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`;
	#echo absoluteFile=$absoluteFile;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"}
	#echo buildFile=$buildFile;
	make ${buildFile}.o;
}
