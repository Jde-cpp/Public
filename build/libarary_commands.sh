#need to add set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG" ) to mysql-concpp/src/mysql-connector-cpp/CMakeLists.txt
cd /mnt/ram/external/Debug
cls;rm -f CMakeCache.txt;cmake /home/duffyj/code/jde/Public/build --preset linux-debug;

DST_DIR=$REPO_DIR/install/g++-14/debug/boost
./bootstrap.sh --prefix=${DST_DIR} --includedir=headers --libdir=dist
echo "using gcc : : /usr/bin/g++-14 ; " >> tools/build/src/user-config.jam
#./b2 --prefix=${DST_DIR} address-sanitizer=on install
./b2 variant=debug --prefix=${DST_DIR} address-sanitizer=on install --with-json --with-charconv

bootstrap.bat --prefix=C:\Users\duffyj\source\repos\libs\install\clang\boost --with-toolset=clang-win
b2.exe toolset=clang --prefix=C:\Users\duffyj\source\repos\libs\install\clang\boost --with-json -q install

bootstrap.bat --prefix=C:\Users\duffyj\source\repos\libs\install\msvc\boost --with-toolset=msvc
b2.exe toolset=msvc address-model=64 address-sanitizer=on --prefix=C:\Users\duffyj\source\repos\libs\install\msvc\multi\boost --with-json --with-headers -q install


DST_DIR=$REPO_DIR/install/g++-14/RelWithDebInfo/boost
./bootstrap.sh --prefix=${DST_DIR} --includedir=headers --libdir=dist --with-libraries=json
./b2 --prefix=${DST_DIR} variant=release inlining=off debug-symbols=on install --with-json --with-charconv

cd /c/Users/duffyj/source/repos/libs/boostorg/boost_1_88_0
destinationDir=$REPO_DIR\\install\\msvc\\boost;
./bootstrap.bat --prefix=${destinationDir};
./b2.exe address-sanitizer=on --prefix=${destinationDir} --with-json -q install;

export CXX=g++-14;
export CC=gcc-14;
export CMAKE_CXX_STANDARD=23;
export CMAKE_POSITION_INDEPENDENT_CODE=ON;
export CMAKE_BUILD_TYPE=Debug;
export BT=$CMAKE_BUILD_TYPE;
export CMAKE_PREFIX_PATH=$REPO_DIR/install/$CXX/Debug;
export CXX_FLAGS="-ggdb -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -D_GLIBCXX_DEBUG -static-libstdc++ -static-libasan -lrt"
#
#asan
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/fmt ../../..

cd $REPO_DIR
git clone https://github.com/fmtlib/fmt.git
cd fmt && mkdir .obj && cd .obj;mkdir $CXX;cd $CXX;mkdir $CMAKE_BUILD_TYPE;cd $CMAKE_BUILD_TYPE;
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/spdlog -DCMAKE_PREFIX_PATH=$REPO_DIR/install/debug -DSPDLOG_FMT_EXTERNAL=1 -DSPDLOG_NO_THREAD_ID=1 -DCMAKE_CXX_FLAGS="-fpermissive" ../..

git clone https://github.com/gabime/spdlog.git;
cd spdlog && mkdir .obj && cd .obj;mkdir $CXX;cd $CXX;mkdir $CMAKE_BUILD_TYPE;cd $CMAKE_BUILD_TYPE;
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/GTest -DCMAKE_C_COMPILER=$CC ../..
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/absl -DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=ON -DABSL_BUILD_TEST_HELPERS=ON -DABSL_LOCAL_GOOGLETEST_DIR=$REPO_DIR/googletest  ../..
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/protobuf -DCMAKE_PREFIX_PATH=$REPO_DIR/install/$BT -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_C_COMPILER=$CC -Dprotobuf_ABSL_PROVIDER=package -Dprotobuf_USE_EXTERNAL_GTEST=ON  ../..
cls;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/duffyj/code/libraries/install/$BT/mysql-connector-cpp
-DCMAKE_POSITION_INDEPENDENT_CODE=1
-DCMAKE_C_COMPILER=$CC
-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -D_GLIBCXX_DEBUG"  ../../..

export CMAKE_CXX_FLAGS=-DCMAKE_CXX_FLAGS=\"$CXX_FLAGS\"
#cmake -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE/$LIB -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD -DCMAKE_POSITION_INDEPENDENT_CODE=$CMAKE_POSITION_INDEPENDENT_CODE -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH $CMAKE_CXX_FLAGS $ARGS ../../..;
cd $JDE_DIR/open62541/.build/g++-13/Debug;
#CMAKE_C_COMPILER:STRING=/usr/bin/$CC
cmake -DCMAKE_INSTALL_PREFIX=/home/duffyj/code/libraries/install/g++-13/asan/open62541 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_PREFIX_PATH=/home/duffyj/code/libraries/install/g++-13/asan -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DCMAKE_CXX_FLAGS="-ggdb -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -D_GLIBCXX_DEBUG -DCMAKE_C_COMPILER=$CC -DUA_ENABLE_ENCRYPTION -static-libstdc++ -static-libasan -lrt" ../../..
#$ cls;rm CMakeCache.txt; cmake -DUA_LOGLEVEL=100 -DUA_ENABLE_ENCRYPTION=OPENSSL ..

export CMAKE_BUILD_TYPE=RelWithDebInfo;
export CMAKE_PREFIX_PATH=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE;

buildLibrary fmtlib/fmt.git;
buildLibrary spdlog -fpermissive "-DSPDLOG_FMT_EXTERNAL=1 -DSPDLOG_NO_THREAD_ID=1";
buildLibrary GTest "" "" googletest;
buildLibrary absl "" "-DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=ON -DABSL_BUILD_TEST_HELPERS=ON -DABSL_LOCAL_GOOGLETEST_DIR=$REPO_DIR/googletest" abseil-cpp;
buildLibrary protobuf "" "-Dprotobuf_ABSL_PROVIDER=package -Dprotobuf_USE_EXTERNAL_GTEST=ON";
buildLibrary mysql-connector-cpp

context-impl=ucontext;


#jsonnet
cls;rm CMakeCache.txt; BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/duffyj/code/libraries/install/g++-13/$BT/jsonnet -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_C_COMPILER=$CC -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath=$ORIGIN" -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"  ../..
cls;make -j8
make install
cd /home/duffyj/code/jde/bin/asan
ln -s /home/duffyj/code/libraries/install/g++-13/asan/jsonnet/lib/libjsonnet.so.0
ln -s /home/duffyj/code/libraries/install/g++-13/asan/jsonnet/lib/libjsonnet++.so.0
