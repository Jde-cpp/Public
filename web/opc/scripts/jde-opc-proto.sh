#!/bin/bash
wsDir=${1:-`pwd`};
clean=${2:-0};
echo jde-opc-proto.sh wsDir=$wsDir clean=$clean;
pushd `pwd` > /dev/null;
pushd `pwd` > /dev/null;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
if ! source $scriptDir/env.sh; then exit 1; fi;
cd $scriptDir/..;

popd > /dev/null;
#after the popd so $wsDir wins over wherever the caller was standing; the cd below and create()'s pbjs lookup are
#both workspace-relative - see the note in jde-framework-proto.sh.
#`||` not `; if [ $? ]` - env.sh installs trap error ERR, which fires on a bare failing command and exits before
#any $? test can run.  Same reason for the nested call below.
cd $wsDir || { echo `pwd`; echo cd $wsDir failed - not a directory; exit 1; };
$frameworkDir/scripts/jde-framework-proto.sh $wsDir $clean || { echo `pwd`; echo jde-framework-proto.sh $wsDir $clean failed; exit 1; };
cd projects/jde-opc/src/lib;
moveToDir proto;

declare -A appFiles;
if isStale Common || [ $clean == 1 ]; then appFiles[Common]=common_root; fi;
create $JDE_BASH/libs/app/shared/proto appFiles $wsDir;
declare -A opcFiles;
if isStale Opc.Common || [ $clean == 1 ]; then opcFiles[Opc.Common]=opc_common_root; fi;
if isStale Opc.FromServer || [ $clean == 1 ]; then opcFiles[Opc.FromServer]=opc_from_server_root; fi;
if isStale Opc.FromClient || [ $clean == 1 ]; then opcFiles[Opc.FromClient]=opc_from_client_root; fi;
create $JDE_BASH/apps/OpcGateway/src/types/proto opcFiles $wsDir;
echo 'jde-opc-proto done';
popd > /dev/null;