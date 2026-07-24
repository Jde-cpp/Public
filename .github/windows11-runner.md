# Self-hosted Windows 11 CI runner

Setup notes for the **Windows 11 x64** workflow
([`.github/workflows/ci-windows11.yml`](workflows/ci-windows11.yml)). Unlike the Linux runner
([`.github/docker/`](docker/README.md)), which is a containerised, ephemeral, auto-registering
runner, the Windows runner is a **bare runner on a real workstation** using the developer toolchain
already installed there. Because it is not sandboxed, the workflow runs **only on manual request**
(`workflow_dispatch`) — never on push/PR.

## Register the runner

Download the runner from repo **Settings → Actions → Runners → New self-hosted runner (Windows
x64)**, then, in an elevated PowerShell in the extracted runner folder:

```powershell
# <reg-token> comes from the "New self-hosted runner" page (expires quickly).
.\config.cmd --url https://github.com/Jde-cpp/Public --token <reg-token> --labels win11-clang

# Install and start as a Windows service so it survives logoff/reboot.
.\svc.cmd install
.\svc.cmd start
```

The `win11-clang` label is what `runs-on: [self-hosted, win11-clang]` targets; GitHub also
auto-adds `self-hosted`, `Windows`, and `X64`. Keep the label distinct from the Linux runner's
`clang22` so a job never lands on the wrong OS. Confirm the runner shows **Idle** under Settings →
Actions → Runners.

**Public-repo safety:** self-hosted + public repo means untrusted code must never execute here.
`workflow_dispatch` can only be triggered by users with write access, so — unlike the Linux
workflow's `push`/`pull_request` triggers — no fork-PR `if` fence is required. Also keep repo
**Settings → Actions → General → Fork pull request workflows** at "require approval for all outside
collaborators".

## Service account requirements

Run the runner service as a user who can **create symbolic links** —
[`build/functions.cmake`](../build/functions.cmake) uses `create_symlink` / `file(CREATE_LINK …
SYMBOLIC)` during configure/build. Options:

- Run the service as `duffyj` (the interactive dev account, which already works), **or**
- grant the service account the `SeCreateSymbolicLinkPrivilege`, **or**
- enable **Developer Mode** (Settings → For developers), which lets non-elevated processes create
  symlinks.

A default SYSTEM-account service without this privilege fails configure with symlink errors.

## Toolchain (on the service PATH)

The `win-clang-debug-jde` preset uses bare compiler names resolved from `PATH`, so the runner
**service** environment (not just your interactive shell) must expose:

- LLVM **clang++** and **ld.lld** 22
- **CMake** ≥ 4.2.3
- **Ninja**
- the Windows SDK providing **`dbgeng.lib`** — the preset links `-ldbgeng` and defines
  `BOOST_STACKTRACE_USE_WINDBG` for Windows stacktraces.

## Prebuilt dependency trees

The `-jde` build does **not** build third-party dependencies; they must already be present
(built with the `win-clang-*-repos` presets):

| Path | Purpose |
|------|---------|
| `%REPO_DIR%\install\clang++\Debug` | fmt / zlib / libxml2 / open62541 / protobuf / abseil / gtest / spdlog / jsonnet / ryml / NodesetLoader — `CMAKE_PREFIX_PATH`. fmt/zlib/libxml2 ship DLLs that CMake POST_BUILD-copies next to each test exe. |
| `%REPO_DIR%\boostorg\boost_1_91_0` | Boost **source** — on Windows `functions.cmake` compiles `boost_json` from source (no `find_package(Boost)`). |
| `%VCPKG_ROOT%\installed` | vcpkg deps for triplet `x64-windows-static-md` (OpenSSL, sqlite3), statically linked. |
| `%UA_NODE_SETS%` | Clone of [OPCFoundation/UA-Nodeset](https://github.com/OPCFoundation/UA-Nodeset) — the OpcServer UALoadTests read it. |

These paths are set explicitly in the workflow `env:` block (the service account may not inherit the
user-level env vars): `REPO_DIR=C:/Users/duffyj/source/repos/libs`,
`VCPKG_ROOT=C:/Users/duffyj/source/repos/libs/vcpkg`,
`UA_NODE_SETS=C:/Users/duffyj/source/repos/libs/UA-Nodeset`.

## Build directory

The workflow sets a **CI-only** `JDE_BUILD_DIR=C:/Users/duffyj/source/build/ci`, kept separate from
the local dev build (`x:\build\clang++\Public\debug`) so the two never fight over one build tree. The
preset appends `\clang++\${sourceDirName}\debug` and the runner always checks out to
`…\_work\Public\Public`, so the actual build dir is
`C:\Users\duffyj\source\build\ci\clang++\Public\debug`.

**Enable long paths** — deep object paths under the CI build root can approach the 260-char limit
(object files embed the full source path):

```powershell
git config --system core.longpaths true
# and set HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled = 1
```

## Notes / trade-offs

- **Incremental builds:** the CI build dir is disk-backed and persistent (not a wiped ramdisk like
  Linux `/mnt/ram`), so Ninja reuses objects across runs — faster, but not a clean build each time.
  Delete `C:\Users\duffyj\source\build\ci` to force a clean rebuild.
- **Antivirus:** Defender scanning thousands of `.obj`/PDB files slows builds and can intermittently
  lock files → flaky link failures. Exclude the CI build root (and LLVM/vcpkg) from real-time scanning.
- **Tests need no DB server:** `addJdeTest` wires the DB-backed suites (access / OpcGateway /
  OpcServer) to in-memory sqlite on every platform, and all runtime DLLs are co-located in
  `<build>\bin`, so no `PATH` tweaks are needed — provided a full `cmake --build` runs before `ctest`.
- **In-source protobuf codegen race (Windows-specific):** the proto libs generate `*.pb.h`/`*.pb.cc`
  *in-source* (`PROTOC_OUT_DIR = <lib>/proto`). On a clean, highly parallel build a consumer compile
  can hold a `*.pb.h` open for read while `protoc` rewrites it — on Windows the share-lock fails
  `protoc` with `"<file>: Invalid argument"`, breaking the build. It is latent on Linux (POSIX permits
  the concurrent open), which is why the Linux CI never sees it. The workflow's Build step tolerates
  this by finishing with a serial `-j 1` pass if the parallel pass fails (race-free; a real compile
  error still fails the serial pass, so nothing is masked). The proper fix is to generate protos
  out-of-source (into the build tree); until then the serial fallback keeps clean Windows builds green.
