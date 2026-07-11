function buildRelativePath() {
	cmakeSourceDir=$1; #/home/duffyj/code/jde/Public2
	absoluteFile=$2; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	relativePath=${absoluteFile#"$cmakeSourceDir/"};
	relativePath=${relativePath%/*};
	filename=$(basename "$absoluteFile");
	if [[ $filename == "main.cpp" ]]; then
		relativePath=${relativePath/src/exe};
	else
		relativePath=${relativePath/src/lib};
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
	cmakeSourceDir=$(realpath $2); #/home/duffyj/code/jde/PublicX
	# buildRoot layout `$1/<repo-basename>` (`$1`=`$JDE_BUILD_DIR/$JDE_COMPILER`) is mirrored in TS by
	# extensions/jde/src/extension.ts repoBuildDir(); a change here must update both. Repeated at 74/93/108.
	buildRoot=$1/$(basename $cmakeSourceDir); # /mnt/ram/jde/clang++/PublicX
	workspaceFolder=$3; #/home/duffyj/code/jde/IotWebsocket/config
	relativeFile=$4; #../tests/BrowseTests.cpp
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	buildRelativePath=`buildRelativePath $cmakeSourceDir $absoluteFile`;

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

	echo cmakeSourceDir=$cmakeSourceDir, buildRoot=$buildRoot, buildRelativePath=$buildRelativePath, target=$target;
	cd $buildRoot;
	cmake --build . -j --target $target;
}
function compile() {
	cmakeSourceDir=$(realpath $2); #/home/duffyj/code/jde/PublicX
	buildRoot=$1/$(basename $cmakeSourceDir); # /mnt/ram/jde/clang++/PublicX
	workspaceFolder=$3; #/home/duffyj/code/jde/IotWebsocket/config
	fileWorkspaceFolder=$4; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests
	relativeFile=$5; #../tests/BrowseTests.cpp
	absoluteFile=`absoluteFile $workspaceFolder $relativeFile`; #/home/duffyj/code/jde/Public2/apps/OpcGateway/tests/BrowseTests.cpp
	buildRelativePath=`buildRelativePath $cmakeSourceDir $absoluteFile`; #libs/web/server

	echo "cmakeSourceDir=$cmakeSourceDir, buildRoot=$buildRoot, workspaceFolder: $workspaceFolder, fileWorkspaceFolder:$fileWorkspaceFolder, relativeFile=$relativeFile, buildRelativePath=$buildRelativePath, absoluteFile=$absoluteFile";
	cd $buildRoot/$buildRelativePath;
	buildFile=${absoluteFile#"$fileWorkspaceFolder/"} #shortest-match prefix removal
	filename=$(basename "$absoluteFile");
	if [[ $filename == "main.cpp" ]]; then
		buildFile=src/main.cpp;
	fi;
	echo $buildRoot/$buildRelativePath/make $buildFile.o;
	make ${buildFile}.o;
}
function reconfig() {
	cmakeDir=$(realpath $2);
	buildRoot=$1/$(basename $cmakeDir);
	preset=$3;
	mkdir -p $buildRoot/runtime/logs;
	cd $buildRoot;
	rm -f CMakeCache.txt;
	tput reset;
	echo `pwd` > cmake.output;
	echo "cmake $cmakeDir -Wno-dev --preset $preset 2>&1" | tee -a cmake.output;
	cmake $cmakeDir -Wno-dev --preset "$preset" 2>&1 | tee -a cmake.output;
	if [ -f "$buildRoot/compile_commands.json" ]; then
		mv $buildRoot/compile_commands.json $cmakeDir/compile_commands.json;
	fi
}
function build() {
	cmakeDir=$(realpath $2);
	buildRoot=$1/$(basename $cmakeDir);
	target=$3;
	cd $buildRoot;
	set -o pipefail;
	echo `pwd`/$target.output > $target.output;
	echo "cmake --build . -j --target $target" | tee -a $target.output;
	cmake --build . -j --target $target | tee -a $target.output;
}
function buildTests() {
	baseTarget=$3;
	if [ $baseTarget == "Jde.Opc.Gateway" ]; then
		baseTarget="Jde.Opc";
	fi;
	build $1 $2 $baseTarget.Tests;
}
