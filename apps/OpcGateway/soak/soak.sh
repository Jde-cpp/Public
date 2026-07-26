#!/usr/bin/env bash
# 24-hour soak test orchestration (MVP.md M1 acceptance gate).
#
# Launches AppServer -> OpcServer -> OpcGateway -> Jde.Opc.Soak as separate processes against fresh file-backed
# sqlite dbs, monitors liveness + RSS for all four, and writes a PASS/FAIL verdict.
#
# Runs under Linux bash and Windows Git Bash (the win11 workstation has no pwsh; the one PowerShell use below is a
# powershell.exe -Command one-liner for Ctrl-C delivery, which bash cannot send to native console apps).
#
#   soak.sh [--duration PT24H] [--run-dir DIR] [--smoke] [--build-dir DIR]
#           [--warmup PT1H] [--quiet-interval PT6H] [--quiet-period PT10M]
#
#   --smoke   10-minute validation run: PT30S status samples, quiet window at 3m for 1m.
#
#   The remaining three exist because the defaults are tuned for the 24h run and do not scale down on their own:
#   --warmup          RSS baseline offset from start (default 1h, 2m under --smoke). MUST be shorter than
#                     --duration or no baseline sample exists and the RSS criterion is silently skipped.
#   --quiet-interval  idle gap between write bursts (client default PT6H - never fires in a run under 6h)
#   --quiet-period    how long each idle window lasts (client default PT10M)
#
# Verdict criteria (verdict.json + exit code):
#   - soak client exit 0 (completed, zero misses/writeFailures/socketDrops/statusFailures)
#   - all three servers alive the whole run and stopped within the 60s grace on request (a SIGINT-stopped app
#     exits 255 - ::pause() returns -1 - so the exit CODE is informational; a hard kill is the failure)
#   - no crash events since start (journalctl+coredumpctl / Application event log)
#   - per-process RSS growth from warmup end to last sample < max(10%, 50MB)

set -uo pipefail

# ---------------------------------------------------------------- args / constants
duration="PT24H"
runDir=""
smoke=0
buildDirOverride=""
warmup=""
quietInterval=""
quietPeriod=""
while [[ $# -gt 0 ]]; do
	case "$1" in
		--duration) duration="$2"; shift 2;;
		--run-dir) runDir="$2"; shift 2;;
		--build-dir) buildDirOverride="$2"; shift 2;;
		--warmup) warmup="$2"; shift 2;;
		--quiet-interval) quietInterval="$2"; shift 2;;
		--quiet-period) quietPeriod="$2"; shift 2;;
		--smoke) smoke=1; duration="PT10M"; shift;;
		*) echo "unknown argument: $1" >&2; exit 2;;
	esac
done

isoSeconds(){ # <PT#H#M#S | plain seconds> -> seconds ; the apps parse ISO-8601, awk/date here do not.
	local v h=0 m=0 s=0
	v=$( echo "$1" | tr '[:lower:]' '[:upper:]' )
	if [[ $v =~ ^[0-9]+$ ]]; then echo "$v"; return 0; fi
	[[ $v =~ ^PT([0-9]+H)?([0-9]+M)?([0-9]+S)?$ && $v != "PT" ]] || { echo "invalid duration '$1' (want PT#H#M#S)" >&2; return 1; }
	[[ -z ${BASH_REMATCH[1]} ]] || h=${BASH_REMATCH[1]%H}
	[[ -z ${BASH_REMATCH[2]} ]] || m=${BASH_REMATCH[2]%M}
	[[ -z ${BASH_REMATCH[3]} ]] || s=${BASH_REMATCH[3]%S}
	echo $(( h*3600+m*60+s ))
}

durationSeconds=$( isoSeconds "$duration" ) || exit 2
warmupSeconds=3600; [[ $smoke -eq 0 ]] || warmupSeconds=120
[[ -z "$warmup" ]] || warmupSeconds=$( isoSeconds "$warmup" ) || exit 2
# Fail here, not 24h later: a baseline at/after the last sample leaves rssVerdict with nothing to compare.
[[ $warmupSeconds -lt $durationSeconds ]] || { echo "warmup ($warmupSeconds s) must be shorter than duration ($durationSeconds s) - pass --warmup" >&2; exit 2; }

isWindows=0
case "${OSTYPE:-}" in msys*|cygwin*) isWindows=1;; esac

scriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$scriptDir/../../.." && pwd)"
repoName="$(basename "$repo")"

if [[ -n "$buildDirOverride" ]]; then
	buildDir="$buildDirOverride"
elif [[ $isWindows -eq 1 ]]; then
	buildDir="${JDE_BUILD_DIR:-/x/build}/clang++-jde/release"          # win-clang-release-jde binaryDir (no sourceDirName)
else
	buildDir="${JDE_BUILD_DIR:-/mnt/ram/linux}/${JDE_COMPILER:-clang++}/$repoName/release" # linux-clang-relWithDebInfo-jde
fi

exeSuffix=""; [[ $isWindows -eq 1 ]] && exeSuffix=".exe"
# np: native path for arguments handed to the apps (jsonnet/sqlite/fstream see native paths, not /c/… MSYS ones).
if [[ $isWindows -eq 1 ]]; then
	np(){ cygpath -m "$1"; }
else
	np(){ echo "$1"; }
fi

findExe(){ # <relative-path-without-suffix> ; windows builds may also drop exes into <buildDir>/bin
	local rel=$1 p
	for p in "$buildDir/$rel$exeSuffix" "$buildDir/bin/$(basename "$rel")$exeSuffix"; do
		[[ -x "$p" ]] && { echo "$p"; return 0; }
	done
	echo "ERROR: $rel$exeSuffix not found under $buildDir (build target $(basename "$rel") first)" >&2
	return 1
}

appServerExe="$(findExe apps/AppServer/exe/Jde.App.Server)" || exit 1
opcServerExe="$(findExe apps/OpcServer/exe/Jde.Opc.Server)" || exit 1
gatewayExe="$(findExe apps/OpcGateway/exe/Jde.Opc.Gateway)" || exit 1
soakExe="$(findExe apps/OpcGateway/soak/Jde.Opc.Soak)" || exit 1

export REPO_SOURCE_DIR="$(np "$repo")"
export REPO_BUILD_DIR="$(np "$(dirname "$buildDir")")"
if [[ -z "${UA_NODE_SETS:-}" ]]; then
	if [[ $isWindows -eq 1 ]]; then export UA_NODE_SETS="C:/Users/duffyj/source/repos/libs/UA-Nodeset";
	else export UA_NODE_SETS="$HOME/code/libs/UA-Nodeset"; fi
fi

runRoot="${JDE_SOAK_DIR:-$HOME/soak-runs}"
[[ -n "$runDir" ]] || runDir="$runRoot/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$runDir"/{appserver,opcserver,gateway,client}/logs "$runDir/db"
procStats="$runDir/procstats.csv"
echo "epoch,time,name,pid,rssBytes" >"$procStats"
startEpoch=$(date +%s)
startIso=$(date -u +%Y-%m-%dT%H:%M:%S)

ulimit -c unlimited 2>/dev/null

cat >"$runDir/manifest.json" <<EOF
{ "sha": "$(git -C "$repo" rev-parse HEAD 2>/dev/null)", "start": "$startIso", "duration": "$duration",
  "buildDir": "$(np "$buildDir")", "os": "$(uname -s)", "smoke": $smoke, "host": "$(hostname)" }
EOF

echo "=== soak run: $runDir (duration $duration, build $buildDir) ==="

# ---------------------------------------------------------------- process helpers
declare -A pids exitCodes cleanStop
stopBroadcast=0 #set once a console-wide Ctrl-C has gone out (windows), so stopAll can tell an expected sibling exit
                #from a premature death.  Stays 0 on linux, where each SIGINT is aimed at one pid.
winpid(){ cat /proc/$1/winpid 2>/dev/null || echo "$1"; }

alive(){ kill -0 "$1" 2>/dev/null; }

rssBytes(){ # <pid>
	if [[ $isWindows -eq 1 ]]; then
		local wp; wp=$(winpid "$1")
		tasklist //FI "PID eq $wp" //FO CSV //NH 2>/dev/null | awk -F'","' 'NF>=5{ gsub(/[^0-9]/,"",$5); print $5*1024 }'
	else
		awk -v ps="$(getconf PAGESIZE)" '{print $2*ps}' /proc/$1/statm 2>/dev/null
	fi
}

stopGraceful(){ # <pid> - SIGINT / console Ctrl-C; caller waits+escalates
	if [[ $isWindows -eq 1 ]]; then
		local wp; wp=$(winpid "$1")
		# Attach to the target's console and broadcast Ctrl-C: the apps route it through SetConsoleCtrlHandler ->
		# the same worker-stop path SIGINT takes on Linux. powershell.exe gets its own console, so FreeConsole is safe.
		#
		# NOT per-process: GenerateConsoleCtrlEvent's group 0 signals every process on the attached console, and MSYS
		# bash hands all its children its own console rather than creating one each - so this reaches all three servers
		# AND this script.  Harmless for teardown (they are all stopping anyway), but the INT mask is required or the
		# script trips its own `trap failEarly INT` and aborts the run.  Callers needing to stop ONE app must hardKill.
		trap '' INT
		powershell.exe -NoProfile -Command "
			Add-Type -Namespace W -Name K -MemberDefinition '
				[DllImport(\"kernel32.dll\")] public static extern bool FreeConsole();
				[DllImport(\"kernel32.dll\")] public static extern bool AttachConsole(uint p);
				[DllImport(\"kernel32.dll\")] public static extern bool SetConsoleCtrlHandler(IntPtr h, bool a);
				[DllImport(\"kernel32.dll\")] public static extern bool GenerateConsoleCtrlEvent(uint e, uint p);';
			[W.K]::FreeConsole() | Out-Null;
			if( [W.K]::AttachConsole($wp) ){
				[W.K]::SetConsoleCtrlHandler([IntPtr]::Zero, \$true) | Out-Null;
				[W.K]::GenerateConsoleCtrlEvent(0, 0) | Out-Null;
			}" >/dev/null 2>&1
		sleep 2 #the event is asynchronous - stay masked until it lands, or the trap fires just after the restore.
		trap 'failEarly "interrupted"' INT
		stopBroadcast=1
	else
		kill -INT "$1" 2>/dev/null
	fi
}

hardKill(){
	if [[ $isWindows -eq 1 ]]; then taskkill //F //PID "$(winpid "$1")" >/dev/null 2>&1; else kill -9 "$1" 2>/dev/null; fi
}

stopAll(){ # graceful teardown: gateway -> opcserver -> appserver; 60s grace each, then hard kill.
	local name pid deadline clean
	for name in gateway opcserver appserver; do
		pid=${pids[$name]:-}
		[[ -n "$pid" ]] || continue
		#Already gone: premature death (unclean) unless a console-wide Ctrl-C has already gone out, in which case this
		#app stopped on that same signal - the expected path on windows, where the broadcast cannot be aimed at one pid.
		#A genuine mid-run death is still caught: the monitor loop clears serversOk before teardown ever starts.
		if ! alive "$pid"; then wait "$pid" 2>/dev/null; exitCodes[$name]=$?; cleanStop[$name]=$stopBroadcast; continue; fi
		echo "stopping $name ($pid)..."
		stopGraceful "$pid"
		deadline=$(( $(date +%s)+60 )); clean=1
		while alive "$pid"; do
			[[ $(date +%s) -lt $deadline ]] || { echo "WARN: $name did not exit in 60s - hard kill"; hardKill "$pid"; clean=0; break; }
			sleep 1
		done
		wait "$pid" 2>/dev/null; exitCodes[$name]=$?
		cleanStop[$name]=$clean
		echo "$name exited: ${exitCodes[$name]} (clean=$clean)"
	done
}

failEarly(){
	echo "FATAL: $1" >&2
	stopAll
	writeVerdict "FAIL" "$1"
	exit 1
}
trap 'failEarly "interrupted"' INT TERM

launch(){ # <name> <exe> <settings> <include> [extra args...]
	local name=$1 exe=$2 settings=$3 include=$4; shift 4
	local cwd="$runDir/$name"
	( cd "$cwd" && exec "$exe" -c -tests "-settings=$(np "$settings")" "-include=$include" "$@" ) >"$runDir/$name/console.log" 2>&1 &
	pids[$name]=$!
	echo "launched $name: pid ${pids[$name]}"
}

waitPort(){ # <name> <port>
	local name=$1 port=$2 i
	for i in $(seq 1 60); do
		alive "${pids[$name]}" || failEarly "$name died during startup - see $runDir/$name/console.log"
		(exec 3<>"/dev/tcp/127.0.0.1/$port") 2>/dev/null && { echo "$name ready on :$port"; return 0; }
		sleep 2
	done
	failEarly "$name did not open :$port within 120s"
}

waitHttp(){ # <name> <url>
	local name=$1 url=$2 i
	for i in $(seq 1 60); do
		alive "${pids[$name]}" || failEarly "$name died during startup - see $runDir/$name/console.log"
		curl -fsS -m 5 "$url" >/dev/null 2>&1 && { echo "$name ready: $url"; return 0; }
		sleep 2
	done
	failEarly "$name did not answer $url within 120s"
}

# ---------------------------------------------------------------- crash + rss verdict helpers
crashEvents(){ # crash records since start touching our binaries -> stdout (empty = none)
	local names='Jde.App.Server|Jde.Opc.Server|Jde.Opc.Gateway|Jde.Opc.Soak'
	if [[ $isWindows -eq 1 ]]; then
		local ageMs=$(( ($(date +%s)-startEpoch+60)*1000 ))
		wevtutil qe Application "//q:*[System[(EventID=1000 or EventID=1001) and TimeCreated[timediff(@SystemTime) <= $ageMs]]]" //f:text 2>/dev/null | grep -E "$names"
	else
		if command -v journalctl >/dev/null 2>&1; then
			journalctl -q --since "@$startEpoch" 2>/dev/null | grep -E "$names" | grep -iE 'terminate|stack|segfault|dumped core'
		fi
		if command -v coredumpctl >/dev/null 2>&1; then
			coredumpctl list --since "@$startEpoch" --no-legend 2>/dev/null | grep -E "$names"
		fi
	fi
	return 0
}

rssVerdict(){ # per-process growth from warmup end to last sample; FAILs > max(10%, 50MB). prints report lines; rc 1 on fail.
	local warmupEnd=$1
	awk -F, -v warmupEnd="$warmupEnd" '
		NR>1 {
			if( $1>=warmupEnd && !($3 in base) ){ base[$3]=$5 }
			last[$3]=$5
		}
		END{
			bad=0
			for( n in last ){
				if( !(n in base) || base[n]<=0 ){ printf "rss %s: no warmup baseline (run shorter than warmup)\n", n; continue }
				growth=last[n]-base[n]; pct=100*growth/base[n]
				limit=base[n]*0.10; if( limit<52428800 ) limit=52428800
				verdict=(growth<=limit) ? "ok" : "FAIL"
				if( verdict=="FAIL" ) bad=1
				printf "rss %s: %.1fMB -> %.1fMB (%+.1f%%) %s\n", n, base[n]/1048576, last[n]/1048576, pct, verdict
			}
			exit bad
		}' "$procStats"
}

verdictWritten=0
writeVerdict(){ # <PASS|FAIL> <reason>
	[[ $verdictWritten -eq 0 ]] || return 0
	verdictWritten=1
	local verdict=$1 reason=$2
	{
		echo "{"
		echo "  \"verdict\": \"$verdict\", \"reason\": \"$reason\","
		echo "  \"clientExit\": ${exitCodes[client]:-null}, \"gatewayExit\": ${exitCodes[gateway]:-null},"
		echo "  \"opcServerExit\": ${exitCodes[opcserver]:-null}, \"appServerExit\": ${exitCodes[appserver]:-null},"
		echo "  \"start\": \"$startIso\", \"end\": \"$(date -u +%Y-%m-%dT%H:%M:%S)\""
		echo "}"
	} >"$runDir/verdict.json"
	echo "=== $verdict - $reason (artifacts: $runDir) ==="
}

# ---------------------------------------------------------------- cert bootstrap (before OpcServer starts!)
# OpcServer snapshots trustedCertDirs at startup; the gateway creates its per-target OPC cert only at first connect.
"$soakExe" -c -tests "-settings=$(np "$scriptDir/config/Opc.Soak.jsonnet")" -createCert >"$runDir/client/createCert.log" 2>&1 \
	|| failEarly "cert bootstrap failed - see $runDir/client/createCert.log"
echo "gateway certificate bootstrapped"

# ---------------------------------------------------------------- launch: AppServer -> OpcServer -> Gateway -> client
launch appserver "$appServerExe" "$scriptDir/config/App.Server.Soak.jsonnet" "../../../AppServer/config/args/sqlite" \
	-arg "path=$(np "$runDir/db/app.db")"
waitPort appserver 1967

launch opcserver "$opcServerExe" "$scriptDir/config/Opc.Server.Soak.jsonnet" "../../../OpcServer/config/args/sqlite" \
	-arg "path=$(np "$runDir/db/opc.db")"
waitPort opcserver 4840

launch gateway "$gatewayExe" "$scriptDir/config/Opc.Gateway.Soak.jsonnet" "../../config/args/sqlite" \
	-arg "path=$(np "$runDir/db/gateway.db")"
waitHttp gateway "http://localhost:1968/ErrorCodes"

# Rights seeding, in two steps because live acl events never update a split-process OpcServer's in-memory rights
# (soak finding): grant AFTER OpcServer's first boot registered the nodeIds resource, then RESTART OpcServer so its
# startup load picks the acl up. First boot may or may not have enabled enforcement (its own registration races
# AssignRights); after the restart, enforcement is deterministically on and the soak user is authorized.
"$soakExe" -c -tests "-settings=$(np "$scriptDir/config/Opc.Soak.jsonnet")" "-include=." -grant >"$runDir/client/grant.log" 2>&1 \
	|| failEarly "rights grant failed - see $runDir/client/grant.log"
echo "soak user granted OPC node access - restarting opcserver to load it"
# Hard kill on windows: a console Ctrl-C cannot be aimed at one process (see stopGraceful), and signalling the whole
# console here would take the AppServer and gateway down mid-startup.  This is a restart, not a shutdown assertion -
# the verdict never reads this stop - and OpcServer reloads its per-run sqlite db from disk on the way back up.
if [[ $isWindows -eq 1 ]]; then hardKill "${pids[opcserver]}"; else stopGraceful "${pids[opcserver]}"; fi
for i in $(seq 1 30); do alive "${pids[opcserver]}" || break; sleep 1; done
alive "${pids[opcserver]}" && { hardKill "${pids[opcserver]}"; sleep 1; }
wait "${pids[opcserver]}" 2>/dev/null
mv "$runDir/opcserver/console.log" "$runDir/opcserver/console.boot1.log" 2>/dev/null
launch opcserver "$opcServerExe" "$scriptDir/config/Opc.Server.Soak.jsonnet" "../../../OpcServer/config/args/sqlite" \
	-arg "path=$(np "$runDir/db/opc.db")"
waitPort opcserver 4840

clientArgs=( "-duration=$duration" "-csv=$(np "$runDir/client/soak.csv")" "-summary=$(np "$runDir/client/summary.json")" )
if [[ $smoke -eq 1 ]]; then
	clientArgs+=( "-statusPeriod=PT30S" )
	[[ -n "$quietInterval" ]] || quietInterval="PT3M"
	[[ -n "$quietPeriod" ]] || quietPeriod="PT1M"
fi
[[ -z "$quietInterval" ]] || clientArgs+=( "-quietInterval=$quietInterval" )
[[ -z "$quietPeriod" ]] || clientArgs+=( "-quietPeriod=$quietPeriod" )
launch client "$soakExe" "$scriptDir/config/Opc.Soak.jsonnet" "." "${clientArgs[@]}"

# ---------------------------------------------------------------- monitor until the client finishes
sampleRss(){
	local name pid rss
	for name in appserver opcserver gateway client; do
		pid=${pids[$name]:-}; [[ -n "$pid" ]] || continue
		alive "$pid" || continue
		rss=$(rssBytes "$pid"); [[ -n "$rss" ]] || continue
		echo "$(date +%s),$(date -u +%H:%M:%S),$name,$pid,$rss" >>"$procStats"
	done
}

serversOk=1
while alive "${pids[client]}"; do
	for name in appserver opcserver gateway; do
		if ! alive "${pids[$name]}"; then
			serversOk=0
			echo "ERROR: $name exited prematurely - see $runDir/$name/console.log" >&2
			break 2
		fi
	done
	curl -fsS -m 5 "http://localhost:1968/ErrorCodes" >/dev/null 2>&1 || echo "WARN: /ErrorCodes ping failed at $(date -u +%H:%M:%S)"
	sampleRss
	for f in "$runDir"/*/console.log "$runDir"/*/logs/*; do
		[[ -f "$f" && $(stat -c%s "$f" 2>/dev/null || echo 0) -gt 2147483648 ]] && echo "WARN: $f exceeds 2GB"
	done
	sleep 15
done

if alive "${pids[client]}"; then hardKill "${pids[client]}"; fi
wait "${pids[client]}" 2>/dev/null; exitCodes[client]=$?
echo "client exited: ${exitCodes[client]}"

# ---------------------------------------------------------------- teardown + verdict
stopAll

failReasons=()
[[ ${exitCodes[client]} -eq 0 ]] || failReasons+=( "client exit ${exitCodes[client]}" )
[[ $serversOk -eq 1 ]] || failReasons+=( "server died mid-run" )
for name in gateway opcserver appserver; do
	[[ ${cleanStop[$name]:-0} -eq 1 ]] || failReasons+=( "$name unclean stop" )
	case "${exitCodes[$name]:-}" in # fatal-signal exits: 134=SIGABRT 135=SIGBUS 139=SIGSEGV (a clean SIGINT stop exits 255)
		134|135|139) failReasons+=( "$name crashed on shutdown (exit ${exitCodes[$name]})" );;
	esac
done

crashes=$(crashEvents)
if [[ -n "$crashes" ]]; then
	echo "$crashes" >"$runDir/crash-events.txt"
	failReasons+=( "crash events recorded (crash-events.txt)" )
fi

rssReport=$(rssVerdict $((startEpoch+warmupSeconds))) || failReasons+=( "RSS growth over limit" )
echo "$rssReport"
echo "$rssReport" >"$runDir/rss-report.txt"

if [[ ${#failReasons[@]} -eq 0 ]]; then
	writeVerdict "PASS" "all criteria met"
	exit 0
else
	writeVerdict "FAIL" "$(IFS='; '; echo "${failReasons[*]}")"
	exit 1
fi
