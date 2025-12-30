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
cd projects/jde-spa/src;
addHardDir highlightjs $controlDir/src;

popd > /dev/null;