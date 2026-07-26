#!/bin/bash

error() {
	local status=$?;   #must be first - it is the trapped command's status
	echo "(${BASH_LINENO[0]}) `pwd`/$BASH_COMMAND";
	#was a hardcoded -9999, which masks to exit 241 - a code that maps to nothing and appears nowhere else in the
	#repo.  Prefer an explicitly-set $errorCode, otherwise propagate the failing command's own status.
	if [ -z "$errorCode" ]; then errorCode=$status; fi;
	if [ "$errorCode" == "0" ]; then errorCode=1; fi;
	exit $errorCode;
}
trap error ERR;