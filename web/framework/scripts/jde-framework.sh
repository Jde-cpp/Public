#!/bin/bash
libRootDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
source $JDE_BASH/build/common.sh;
pushd `pwd` > /dev/null;

if [ ! -d node_modules/@material-ui/core ]; then npm install @material-ui/core --save; fi;
if [ ! -d node_modules/@material-ui/icons ]; then npm install @material-ui/icons --save; fi;

popd > /dev/null;