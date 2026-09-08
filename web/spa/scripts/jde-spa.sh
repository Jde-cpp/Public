#!/bin/bash
mainDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
source $JDE_BASH/build/common.sh
controlDir=$mainDir/control;
pushd `pwd` > /dev/null;

workspace=$(basename $PWD);
stylesPath=".projects.\"$workspace\".architect.build.options.styles";
styles=("azure-blue" "cyan-orange" "magenta-violet" "rose-red");
stylesContent="[";
for (( i=0; i<${#styles[@]}; ++i )); do
	stylesContent=$stylesContent"{\"inject\": false,\"input\": \"projects/jde-spa/src/styles/custom-themes/${styles[$i]}.scss\",\"bundleName\": \"${styles[$i]}\" },";
done;
stylesContent=$stylesContent\"src/styles.scss\"];
jq "$stylesPath = $stylesContent" angular.json > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq \"$stylesPath = $stylesContent\" angular.json; exit 1; fi;
mv temp.json angular.json;
#jde-spa imports marked (help pages).  A peer of the library, so the workspace has to install it - and create-workspace.sh's
#install block only runs when the workspace is first created, so an existing one never sees it.  Dynamic import only:  a
#static one would hoist it into the initial bundle (see web/CLAUDE.md).
if ! jq -e '.dependencies.marked' package.json > /dev/null; then
	npm --silent install marked@^18.0.0 || { echo `pwd`; echo npm install marked failed; exit 1; };
fi;
cd projects/jde-spa/src;
addHardDir highlightjs $controlDir/src;

popd > /dev/null;