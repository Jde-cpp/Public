#!/bin/bash

error() {
  echo "(${BASH_LINENO[0]}) `pwd`/$BASH_COMMAND";
	if [ -z "$errorCode" ]; then errorCode=-9999; fi;
    exit $errorCode;
}
trap error ERR;