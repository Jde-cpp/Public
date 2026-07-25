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

#Directory counterpart of mklink.  It cannot share mklink's body: cmd mklink needs /D for a directory, and the
#existence tests differ.  /D is a real symlink, matching the file mklink above and the workspace's preserveSymlinks;
#swap to /J (junction) if creating links without Developer Mode/elevation turns out to matter.
#Only an existing *symlink* is replaced - a real directory is left for the caller, so this never deletes a source
#tree through a link.
function mklinkDir {
	local dir=$1;
	local fetchLocation=$2;
	if [ -L $dir ]; then rm $dir; fi;
	if windows; then
		#test the bash-side path - the converted one is only good as an argument to cmd, not to test.
		if [ ! -d "$fetchLocation/$dir" ]; then echo $PS4 $fetchLocation/$dir not found; exit 1; fi;
		toWinDir "$fetchLocation" _source;
		toWinDir "`pwd`" _destination;
		cmd <<< "mklink /D \"$_destination\\$dir\" \"$_source\\$dir\" " > /dev/null;  #"
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo mklink /D "$_destination\\$dir" "$_source\\$dir";
			exit 1;
		fi;
	else
		ln -s $fetchLocation/$dir .;
	fi;
}

function moveToDir {
	local dir=$1;
	mkdir -p $dir; cd $dir;
}

function toBashDir {
	windowsDir=$1;
 	local -n _bashDir=$2
 	_bashDir=${windowsDir/:/}; _bashDir=${_bashDir//\\//};
 	if [[ $_bashDir == C/* || $_bashDir == C ]]; then
 		_bashDir="c${_bashDir:1}"
 	fi
	if [[ ${_bashDir:0:1} != "/" ]]; then _bashDir=/$_bashDir; fi;
}

function toWinDir {
	bashDir=$1;
	local -n _winDir=$2
	_winDir=${bashDir////\\};
	if [[ $_winDir == \\c\\* ]]; then _winDir=c:${_winDir:2};
	elif [[ $_winDir == \"\\c\\* ]]; then _winDir=\"c:${_winDir:3}; fi;
}
