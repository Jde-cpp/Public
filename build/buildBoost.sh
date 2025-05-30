target=${1:debug}
compiler=${2:-msvc}
source common-error.sh;
source common.sh;

echo ------------------------Build Boost-------------------------
if [[ -z $BOOST_DIR ]]; then echo Please define BOOST_DIR.; exit 1; fi;
toBashDir $BOOST_DIR boostBashDir;
echo boostBashDir=$boostBashDir;

if [[ -z $REPO_DIR ]]; then echo Please define REPO_DIR.; exit 1; fi;
toBashDir $REPO_DIR repoBashDir;
echo repoBashDir=$repoBashDir;

pushd `pwd` >> /dev/null;
cd $boostBashDir;

destDir=$repoBashDir/install/$compiler/boost;
echo destDir=$destDir;

if windows; then
	if [[ $target == "debug" ]]; then
		if [[ ! -f $destDir/lib/libboost_json-vc143-mt-gd-x64-1_88.lib ]]; then
			./bootstrap.bat --prefix=${destDir};
			./b2.exe address-sanitizer=on --prefix=$destDir --with-json -q install;
		else
			echo "Already built";
		fi;
	else
		if [[ ! -f $destDir/lib/libboost_json-vc143-mt-x64-1_88.lib ]]; then
			./bootstrap.bat --prefix=${destDir};
			#libboost_json-vc143-mt-x64-1_88.lib = release
			./b2.exe variant=release debug-symbols=on install -q --prefix=$destDir --with-json;
		else
			echo "Already built";
		fi;
	fi;
else
	./bootstrap.sh --prefix=${debugDir} --includedir=headers --libdir=dist --with-libraries=json;
  echo "using gcc : : /usr/bin/g++-14 ; " >> tools/build/src/user-config.jam
	./b2 --prefix=${debugDir} install
fi;
echo --------------------Boost complete---------------------
popd >> /dev/null;



