#!/bin/bash
#Angular cli helpers shared by create-workspace.sh (which needs the global cli's version to pin the workspace it
#scaffolds) and update-angular.sh (which keeps that version current).  Source build/common.sh first - windows() below
#comes from there.

#True when version $1 is older than version $2 - an empty $1 (not installed at all) counts as older.
#`sort -V` orders by version rather than lexically: 22.10.0 sorts after 22.9.0, which a string compare gets backwards.
#Asks "is $2 the newer of the two" rather than "is $1 not the newer", so equal versions answer false.
#Only call this from an if/&&/|| condition - the `return 1` is an answer, not a failure.
function isOutdated {
	local installed=$1; local latest=$2;
	if [ -z "$installed" ]; then return 0; fi;
	if [ "$installed" == "$latest" ]; then return 1; fi;
	[ "`printf '%s\n%s\n' "$installed" "$latest" | sort -V | tail -1`" == "$latest" ];
}

#The version of the globally installed @angular/cli, into the variable named by $1 - empty when there is no global cli.
#Read off the package.json next to the resolved `ng` rather than out of `ng --version`: inside a workspace the shim
#hands over to the workspace-local cli and reports that version instead of the global one.
#`which ng` resolves through the symlink to <pkg>/bin/ng.js on linux, so the manifest is one level up.  on windows
#npm writes a plain sh shim into the prefix itself and readlink -f returns that shim, leaving <prefix>/../package.json
#pointing at nothing - there the package sits under <prefix>/node_modules.  try both, and match on .name rather than
#mere existence, since <prefix>/../package.json can be an unrelated manifest.
function ngGlobalVersion {
	local -n _ngVersion=$1;
	_ngVersion=;
	local ngExe=`which ng 2> /dev/null`;
	if [ ! -x "$ngExe" ]; then return 0; fi;
	local ngPath=`readlink -f "$ngExe"`;
	local candidate;
	for candidate in "`dirname $ngPath`/.." "`dirname $ngPath`/node_modules/@angular/cli"; do
		if [ "`jq -r .name "$candidate/package.json" 2> /dev/null`" == "@angular/cli" ]; then
			_ngVersion=`jq -er .version "$candidate/package.json" 2> /dev/null`;
			return 0;
		fi;
	done;
}

#`npm install -g $1`, elevated only when it has to be.  A distro node puts the global prefix under /usr and needs
#root; nvm/fnm put it under $HOME and do not - and there `sudo npm` is worse than unnecessary, because sudo resets
#PATH to secure_path, which the version-manager shim is not on, so npm is simply not found.  Test the directory that
#is about to be written rather than assuming either layout.
function npmInstallGlobal {
	local package=$1;
	local prefix=`npm config get prefix`;
	#linux/mac keep global packages in <prefix>/lib/node_modules, windows in <prefix>/node_modules.
	local modules=$prefix/lib/node_modules;
	if [ ! -d "$modules" ]; then modules=$prefix/node_modules; fi;
	local cmd="npm install -g $package";
	echo $cmd;
	if windows || [ -w "$modules" ]; then
		$cmd;
	else
		sudo $cmd;
	fi;
}
