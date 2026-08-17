#!/bin/bash
#Bring the globally installed @angular/cli up to the latest published release - what the `sudo npm install -g
#@angular/cli@latest` note at the top of create-workspace.sh asked for by hand, as a check instead.
#It matters because create-workspace.sh installs the global cli only when it is missing and then pins everything it
#scaffolds to whatever version that turned out to be (`ng new` writes it, and the @angular/material install takes its
#major), so the global cli decides which Angular a newly created workspace lands on.  Run this before creating one.
#usage: update-angular.sh [-check] [workspaceDir]
#  -check         report what is out of date and exit 1 if anything is - install nothing.
#  workspaceDir   also update that workspace's @angular/* packages, via its own `ng update` so the migrations run.
#                 An existing workspace is left alone unless named: `ng update` rewrites its package.json and
#                 node_modules in place, which is the workspace's own concern (create-workspace.sh does not touch an
#                 existing workspace either - each concern owns its regen entry point).
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )";
source $JDE_BASH/build/common.sh;
source $scriptDir/common-ng.sh;
#the workspace half updates these four together: `ng update` runs one migration set across the packages it is given,
#and material/cdk track the same major as the cli (create-workspace.sh adds them with `ng add @angular/material@$ngMajor`).
ngPackages=( @angular/cli @angular/core @angular/cdk @angular/material );

check=0;
wsDir=;
for arg in "$@"; do
	case $arg in
		-check|--check) check=1;;
		-*) echo $PS4 unknown flag $arg; echo "usage: update-angular.sh [-check] [workspaceDir]"; exit 1;;
		*) wsDir=$arg;;
	esac;
done;
findExecutable npm;
findExecutable jq;
#validated before anything is installed, so a mistyped path does not first drag the global cli forward.
if [ -n "$wsDir" ]; then
	if [ ! -d $wsDir ]; then echo `pwd`; echo $wsDir is not a directory; exit 1; fi;
	if [ ! -f $wsDir/angular.json ]; then echo `pwd`; echo $wsDir is not an angular workspace - no angular.json; exit 1; fi;
fi;

outdated=0;
latest=`npm view @angular/cli version 2> /dev/null`;
if [ -z "$latest" ]; then echo `pwd`; echo could not read the published @angular/cli version - is the npm registry reachable?; exit 1; fi;
ngGlobalVersion installed;
echo "global @angular/cli ${installed:-none} - latest $latest";
if ! isOutdated "$installed" "$latest"; then
	#a global cli ahead of the registry (a prerelease, a local pack) is deliberate, and installing the `latest` tag over
	#it would be a silent downgrade - isOutdated only answers true for a genuine upgrade.
	echo global @angular/cli is up to date;
elif [ $check -ne 0 ]; then
	outdated=1;
else
	#install the resolved version, not the `latest` tag, so what lands is the version that was just reported.
	npmInstallGlobal @angular/cli@$latest || { echo `pwd`; echo npm install -g @angular/cli@$latest failed; exit 1; };
	#npm exits 0 for an install that went to a different prefix than the one `ng` resolves out of, so re-read it.
	ngGlobalVersion installed;
	if [ "$installed" != "$latest" ]; then echo `pwd`; echo npm reported success but ng is still ${installed:-missing}; exit 1; fi;
	echo "@angular/cli $installed installed";
fi;

if [ -n "$wsDir" ]; then
	cd $wsDir || { echo `pwd`; echo cd $wsDir failed; exit 1; };
	#`ng update` errors on any package that is not a dependency, and material/cdk are added by create-workspace.sh's
	#`ng add` rather than by `ng new`, so ask package.json instead of assuming the set.
	declare -a packages=();
	for package in "${ngPackages[@]}"; do
		jq -e ".dependencies.\"$package\" // .devDependencies.\"$package\"" package.json &> /dev/null && packages+=($package);
	done;
	if [ ${#packages[@]} -eq 0 ]; then echo `pwd`; echo no @angular packages in $wsDir/package.json; exit 1; fi;
	#Compare the installed versions ourselves rather than running a bare `ng update` for its report: that only flags
	#what falls outside package.json's ranges, so a workspace on 22.0.6 with a "^22.0.6" range reports "everything seems
	#to be in order" while 22.1.4 sits published.  Naming the packages explicitly on the update line below does pick
	#those up - it resolves each to @latest - so the report has to agree with what the update would actually do.
	declare -a stale=();
	for package in "${packages[@]}"; do
		pkgInstalled=`jq -er .version node_modules/$package/package.json 2> /dev/null`;
		pkgLatest=`npm view $package version 2> /dev/null`;
		if [ -z "$pkgLatest" ]; then echo `pwd`; echo could not read the published $package version; exit 1; fi;
		if isOutdated "$pkgInstalled" "$pkgLatest"; then
			echo "$wsDir $package ${pkgInstalled:-none} - latest $pkgLatest";
			stale+=($package);
		fi;
	done;
	if [ ${#stale[@]} -eq 0 ]; then
		echo $wsDir @angular packages are up to date;
	elif [ $check -ne 0 ]; then
		outdated=1;
	else
		#the workspace's own cli, not the global one: `ng update` runs the migrations that ship with the version being
		#updated *from*, and a workspace can legitimately sit a major behind the global cli.
		ng=node_modules/.bin/ng;
		if [ ! -x $ng ]; then echo `pwd`; echo $wsDir/node_modules/.bin/ng not found - npm install there first; exit 1; fi;
		#every named package updates together, not just the stale ones: `ng update` refuses a partial @angular/* set
		#across a major, and the migrations it runs are versioned for the whole framework.
		#--allow-dirty: the workspace is generated and git-ignored (.gitignore **/my-workspace*/**), so ng's cleanliness
		#check only ever sees unrelated changes in the surrounding repo and would refuse on every run.
		echo $ng update "${packages[@]}" --allow-dirty;
		$ng update "${packages[@]}" --allow-dirty || { echo `pwd`; echo $ng update "${packages[@]}" failed; exit 1; };
	fi;
fi;
if [ $outdated -ne 0 ]; then echo update-angular.sh: updates available.; else echo update-angular.sh complete.; fi;
exit $outdated;
