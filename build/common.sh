windows() { [[ -n "$WINDIR" ]]; }

if windows; then
  compiler=msvc;
else
  compiler=g++14;
fi;

function addHard {
	local file=$1;#TwsSocketClient64.vcxproj
	local fetchLocation=$2;
	if [ -f $file ]; then rm $file; fi;
	if windows; then
		toWinDir "$fetchLocation" _source;
		toWinDir "`pwd`" _destination;
		cmd <<< "mklink /H \"$_destination\\$file\" \"$_source\\$file\" " > /dev/null; #"
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" "; #"
			exit 1;
		fi;
	else
		ln $fetchLocation/$file .;
	fi;
};

function addHardDir {
	local dir=$1;
	local sourceDir=$2/$1;
	moveToDir $dir;
	for filename in $sourceDir/*; do
		if [ -f $filename ]; then addHard $(basename "$filename") $sourceDir;
		elif [ -d $filename ]; then addHardDir $(basename "$filename") $sourceDir; fi;
	done;
	cd ..;
}

function findExecutable {
	exe=$1;
	defaultPath=$2;
	exitFailure=${3:-1};
	local path_to_exe=$(which "$exe" 2> /dev/null);
	if [ ! -x "$path_to_exe" ]; then
		if  [[ -x "${defaultPath//\\}/$exe" ]]; then
     	PATH=${defaultPath//\\}:$PATH;
		else
			if [ $exitFailure -eq 1 ]; then
				echo `pwd`;
				echo common.sh:?? can not find "${defaultPath//\\}/$exe";
				exit 1;
			fi;
		fi;
	fi;
}

function mklink {
	local file=$1;
	local fetchLocation=$2;
	if [ -f $file ]; then rm $file; fi;
	if windows; then
		toWinDir "$fetchLocation" _source;
		if [ ! -f "$_source/$file" ]; then echo $PS4 $_source/$file not found; exit 1; fi;
		toWinDir "`pwd`" _destination;
		cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" " > /dev/null;  #"
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" "; #"
			exit 1;
		fi;
	else
 	if [ -L $file ]; then rm $file; fi;
		ln -s $fetchLocation/$file .;
	fi;
}

function moveToDir {
	local dir=$1;
	mkdir -p $dir; cd $dir;
}

function toBashDir {
	windowsDir=$1;
 	local -n _bashDir=$2
 	_bashDir=${windowsDir/:/}; _bashDir=${_bashDir//\\//}; _bashDir=${_bashDir/C/c};
	if [[ ${_bashDir:0:1} != "/" ]]; then _bashDir=/$_bashDir; fi;
}

function toWinDir {
	bashDir=$1;
	local -n _winDir=$2
	_winDir=${bashDir////\\};
	if [[ $_winDir == \\c\\* ]]; then _winDir=c:${_winDir:2};
	elif [[ $_winDir == \"\\c\\* ]]; then _winDir=\"c:${_winDir:3}; fi;
}
