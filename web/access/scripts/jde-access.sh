#!/bin/bash
if [ ! -d node_modules ]; then echo must run from angular dir; exit 1; fi;
angularDir=`pwd`;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
cd $scriptDir/..;
baseDir=`pwd`;
source $baseDir/../../../Framework/scripts/common.sh;

#cd $angularDir/src;
#addHard styles.scss $baseDir/site;
#addHard index.html $baseDir/site;
#addHard favicon.ico $baseDir/site;

#cd $angularDir/src/app;
#echo `pwd`;
#addHard app.module.ts $baseDir/site/app;
#addHard app.component.scss $baseDir/site/app;
#addHard app.component.html $baseDir/site/app;
#addHard app.component.ts $baseDir/site/app;

cd $angularDir;