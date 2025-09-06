#!/bin/bash
clean=${1:-0};
shouldFetch=${2:0};
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $scriptDir;
source env.sh;
baseWebDir=$JDE_BASH/Public/web;
cd ..;
cmd="../framework/scripts/create-workspace.sh my-workspace $baseWebDir/spa $baseWebDir/framework $baseWebDir/access $baseWebDir/opc";
echo $cmd
$cmd; if [ $? -ne 0 ]; then echo `pwd`; echo $cmd; exit 1; fi;
cd my-workspace/src;
sitePath=`realpath $scriptDir/../site`;
rm main.ts;
addHard main.ts $sitePath;
addHard styles.scss $sitePath;
addHard index.html $sitePath;
addHard favicon.ico $sitePath;
cd app;
echo `pwd`: addHard app_routing_module.ts $sitePath/app;
addHard app_routing_module.ts $sitePath/app;
addHard app.component.html $sitePath/app;
addHard app.component.scss $sitePath/app;
addHard app.component.ts $sitePath/app;
addHard app.module.ts $sitePath/app;
rm -f app.config.ts;
addHard app.config.ts $sitePath/app;
moveToDir services;
addHard environment.service.ts $sitePath/app/services;
cd ../..;
moveToDir environments;
addHard environment.ts $sitePath/environments;
echo ------------------- Starting Build -------------------;
ng build --output-hashing=none --source-map=true;