source common-error.sh;
source common.sh;

echo ---------------------Build Boost----------------------
if [[ -z $BOOST_DIR ]]; then echo Please define BOOST_DIR.; exit 1; fi;
echo BOOST_DIR=$BOOST_DIR;

if [[ -z $REPO_DIR ]]; then echo Please define REPO_DIR.; exit 1; fi;
echo REPO_DIR=$REPO_DIR;

pushd `pwd` >> /dev/null;
cd $BOOST_DIR;

destinationDir=$REPO_DIR/install/$compiler;
echo destinationDir=$destinationDir;
debugDir=${destinationDir}/Debug/boost;
releaseDir=${destinationDir}/RelWithDebInfo/boost;

if windows then
	./bootstrap.bat --prefix=${debugDir} --includedir=headers --libdir=dist --with-libraries=json;
	./b2.exe --prefix=${debugDir} --with-json -q install;
	echo -----------------Debug Boost complete-----------------
	./b2.exe --prefix=${RreleaseDir} --with-json variant=release inlining=off debug-symbols=on install;
else
	./bootstrap.sh --prefix=${debugDir} --includedir=headers --libdir=dist --with-libraries=json;
  echo "using gcc : : /usr/bin/g++-14 ; " >> tools/build/src/user-config.jam
	./b2 --prefix=${debugDir} install
fi;
echo ----------------Release Boost complete----------------
popd >> /dev/null;



