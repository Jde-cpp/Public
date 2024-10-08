export CXX=g++-13;
export CC=gcc-13;
export CMAKE_CXX_STANDARD=23;
export CMAKE_POSITION_INDEPENDENT_CODE=ON;
export CMAKE_BUILD_TYPE=Debug;
export CMAKE_PREFIX_PATH=$REPO_DIR/install/$CXX/asan;
export CXX_FLAGS="-ggdb -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -D_GLIBCXX_DEBUG -static-libstdc++ -static-libasan -lrt"
#
#asan
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/fmt ../..
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/spdlog -DCMAKE_PREFIX_PATH=$REPO_DIR/install/debug -DSPDLOG_FMT_EXTERNAL=1 -DSPDLOG_NO_THREAD_ID=1 -DCMAKE_CXX_FLAGS="-fpermissive" ../..
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/GTest -DCMAKE_C_COMPILER=gcc-13 ../..
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/absl -DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=ON -DABSL_BUILD_TEST_HELPERS=ON -DABSL_LOCAL_GOOGLETEST_DIR=$REPO_DIR/googletest  ../..
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=$BT -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$BT/protobuf -DCMAKE_PREFIX_PATH=$REPO_DIR/install/$BT -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_C_COMPILER=gcc-13 -Dprotobuf_ABSL_PROVIDER=package -Dprotobuf_USE_EXTERNAL_GTEST=ON  ../..
cls;BT=asan;cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/duffyj/code/libraries/install/$BT/mysql-connector-cpp -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -D_GLIBCXX_DEBUG"  ../../..

export CMAKE_CXX_FLAGS=-DCMAKE_CXX_FLAGS=\"$CXX_FLAGS\"
#cmake -DCMAKE_INSTALL_PREFIX=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE/$LIB -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD -DCMAKE_POSITION_INDEPENDENT_CODE=$CMAKE_POSITION_INDEPENDENT_CODE -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH $CMAKE_CXX_FLAGS $ARGS ../../..;
cd $JDE_DIR/open62541/.build/g++-13/Debug;
cmake -DCMAKE_INSTALL_PREFIX=/home/duffyj/code/libraries/install/g++-13/asan/open62541 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_PREFIX_PATH=/home/duffyj/code/libraries/install/g++-13/asan -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DCMAKE_CXX_FLAGS="-ggdb -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -D_GLIBCXX_DEBUG -DUA_ENABLE_ENCRYPTION -static-libstdc++ -static-libasan -lrt" ../../..
#$ cls;rm CMakeCache.txt; cmake -DUA_LOGLEVEL=100 -DUA_ENABLE_ENCRYPTION=OPENSSL ..

export CMAKE_BUILD_TYPE=RelWithDebInfo;
export CMAKE_PREFIX_PATH=$REPO_DIR/install/$CXX/$CMAKE_BUILD_TYPE;

buildLibrary fmt;
buildLibrary spdlog -fpermissive "-DSPDLOG_FMT_EXTERNAL=1 -DSPDLOG_NO_THREAD_ID=1";
buildLibrary GTest "" "" googletest;
buildLibrary absl "" "-DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=ON -DABSL_BUILD_TEST_HELPERS=ON -DABSL_LOCAL_GOOGLETEST_DIR=$REPO_DIR/googletest" abseil-cpp;
buildLibrary protobuf "" "-Dprotobuf_ABSL_PROVIDER=package -Dprotobuf_USE_EXTERNAL_GTEST=ON";
buildLibrary mysql-connector-cpp

context-impl=ucontext;
b2 cxxstd=23 cxxflags="$CXX_FLAGS" -j 8 --prefix=$REPO_DIR/install/$CXX/asan/boost --build-type=complete --layout=tagged
#--with-atomic --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --with-test --with-locale --with-iostreams --with-log --with-timer --with-exception
