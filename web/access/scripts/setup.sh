#!/bin/bash
clean=${1:-0};
branch=${2:-master};
shouldFetch=${2:-0};
scriptsDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
if ! source $scriptsDir/env.sh; then exit 1; fi;
cd $webDir;
fetchDir $branch WebFramework $shouldFetch;
fetchDir $branch MaterialSite $shouldFetch;
cd $appRootDir;

cmd="$frameworkDir/scripts/create-workspace.sh my-workspace $webDir/MaterialSite $webDir/WebFramework $appRootDir";
echo $cmd;
$cmd;
if [ $? -ne 0 ]; then echo `pwd`; echo $cmd; exit 1; fi;
cd my-workspace;
##ng build --output-hashing=none --source-map=true;

cd src;
sitePath=`realpath $scriptsDir/../site`;
##rm main.ts;
addHard main.ts $sitePath;
addHard styles.scss $sitePath;
addHard index.html $sitePath;
addHard favicon.ico $sitePath;
cd app;
addHard app_routing_module.ts $sitePath/app;
addHard app.component.html $sitePath/app;
addHard app.component.scss $sitePath/app;
addHard app.component.ts $sitePath/app;
addHard app.module.ts $sitePath/app;
addHard app.config.ts $sitePath/app;
moveToDir services;
addHard environment.service.ts $sitePath/app/services;
cd ../..;
moveToDir environments;
addHard environment.ts $sitePath/environments;