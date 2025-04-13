#!/bin/bash
buildTarget=${1:-Debug};
clean=${2:-0};
buildDir=${3:-$JDE_BUILD_DIR}/Public;

sourceDir=$JDE_DIR/Public;

windows() { [[ -n "$WINDIR" ]]; }

if windows; then
	preset="win-jde-${buildTarget,,}";
else
	preset="linux-jde-${buildTarget,,}";
fi
mkdir -p $buildDir/libs/crypto/$buildTarget;
cd $buildDir/libs/crypto/$buildTarget;
if [ $clean -eq 1 ]; then
	rm -rf *;
fi
cmake $sourceDir/libs/crypto --preset $preset;
make -j; if [ $? -ne 0 ]; then echo "Build Failed"; echo `pwd`; exit 1; fi;
ctest --verbose;
