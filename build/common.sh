windows() { [[ -n "$WINDIR" ]]; }

if windows; then
  compiler=msvc;
else
  compiler=g++14;
fi;

function toBashDir {
	windowsDir=$1;
 	local -n _bashDir=$2
 	_bashDir=${windowsDir/:/}; _bashDir=${_bashDir//\\//}; _bashDir=${_bashDir/C/c};
	if [[ ${_bashDir:0:1} != "/" ]]; then _bashDir=/$_bashDir; fi;
}

function moveToDir {
	local dir=$1;
	mkdir -p $dir; cd $dir;
}