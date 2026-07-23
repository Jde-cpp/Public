# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a monorepo for the **Jde OpcGateway** system — an OPC-UA gateway with REST/WebSocket API, Angular frontend, and C++ backend services. Key components:

- `libs/fwk/` — Core C++ framework library (`libJde.so`)
- `libs/db/`, `libs/access/`, `libs/web/`, `libs/opc/`, `libs/ql/`, `libs/app/` — Supporting libraries
- `apps/AppServer/` — Application server (logging, message transfer)
- `apps/OpcGateway/` — OPC-UA gateway (main product, exposes REST/WebSocket)
- `apps/OpcServer/` — OPC server bridge
- `include/jde/fwk/` — Public C++ headers for the framework
- `web/opc/my-workspace/` — Angular 22 frontend (the active workspace)

## Building (C++)

Build outputs go to `/mnt/ram/`. `$JDE_DIR` (= `$JDE_BASH`) is this checkout's source root; `$REPO_DIR` is the **third-party** root (`/home/duffyj/code/libs`) that the presets install dependencies under (`installRoot = $env{REPO_DIR}/install/$env{CXX}`) — it is not the jde repo. Each compiler+checkout pair gets an out-of-source build directory per build type: `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>/<debug|release>` (e.g. `/mnt/ram/linux/clang++/Public/debug`). The helpers in `build/buildFunctions.sh` take that full build dir as their first argument; `reconfig` creates it (plus `runtime/logs`) and copies the generated `compile_commands.json` to the source root for clangd (the build dir keeps its own copy too, for tools like VS Code's CMake Tools that expect it there).

```bash
source $JDE_DIR/build/buildFunctions.sh
buildDir=$JDE_BUILD_DIR/$JDE_COMPILER/$(basename $JDE_DIR)/debug   # /mnt/ram/linux/clang++/Public/debug

# Configure (wipes CMakeCache.txt, creates runtime/logs, copies compile_commands.json to the source root)
reconfig $buildDir $JDE_DIR linux-clang-debug-jde

# Build a target
build $buildDir $JDE_DIR Jde
build $buildDir $JDE_DIR Jde.Fwk.Tests

# Compile one file (resolves the object target from the build tree)
compile $buildDir $JDE_DIR/libs/fwk/src/io/json.cpp

# Raw cmake equivalents. -B is required: no Linux preset sets binaryDir, and preset mode ignores cwd.
cmake -B $buildDir -S $JDE_DIR -Wno-dev --preset linux-clang-debug-jde
cmake --build $buildDir -j --target Jde
cmake --install $buildDir
```

Linux uses **clang++-22** with libc++ (plus ASan/LSan in debug); Windows uses clang too. C++ standard is **C++26**. Every selectable configure preset (`cmake --list-presets`) carries one of two suffixes; everything else in `CMakePresets.json` is a `hidden` building block.

**`-jde`** — builds the project (libs, apps, tests: `jde_TESTS=ON`, `jde_APPS=ON`), adding `CMAKE_EXPORT_COMPILE_COMMANDS` and the house warning exclusions. This is the day-to-day dev build:

| compiler | debug | relWithDebInfo |
|---|---|---|
| clang | `linux-clang-debug-jde` | `linux-clang-relWithDebInfo-jde` |
| win clang | `win-clang-debug-jde` | — |

**`-repos`** — builds only the third-party dependencies (`jde_REPOS=ON`, `jde_TESTS=OFF`, `jde_APPS=OFF`):

| compiler | debug | relWithDebInfo |
|---|---|---|
| clang | `linux-clang-debug-repos` | `linux-clang-relWithDebInfo-repos` |
| win clang | `win-clang-debug-repos` | — |
| g++ | `linux-debug-repos` | `linux-relWithDebInfo-repos` |

The g++ (`linux-*`, **g++-15**) presets exist only for the dependency build — there is no g++ `-jde` preset, so the project itself is built with clang. Note the leftover `linux-debug` **build**/**test** presets are currently unusable: they name the now-hidden `linux-debug` configure preset, and CMake rejects that (`Cannot use hidden configure preset`) — see `reviews/todo.md` §3.

## Running Tests (C++)

Tests use **GoogleTest**. Each test binary requires a `-settings=` argument pointing at a Jsonnet config. Per-library configs live at `libs/<lib>/tests/config/<Lib>.Tests.jsonnet` (and `apps/<app>/tests/config/<App>.Tests.jsonnet`):

- `libs/fwk/tests/config/Framework.Tests.jsonnet`
- `libs/db/drivers/sqlite/tests/config/Sqlite.Tests.jsonnet` (in-process, no db server needed)
- `libs/access/tests/config/Access.Tests.jsonnet`
- `libs/web/tests/config/Web.Tests.jsonnet`
- `apps/OpcGateway/tests/config/Opc.Tests.jsonnet`
- `apps/OpcServer/tests/config/Opc.Server.Tests.jsonnet`

A **test-mode flag is required** — `-tests` for direct runs, `-ctest` for ctest. They are equivalent except that `-ctest` also selects a compact console log pattern (`log/SpdLog.cpp`). One of them binds the `buildTarget`/`cwd`/`logsDir`/`windows` ext vars the configs read, and selects the default import dir — `config/args/mysql` on Linux, `config/args/sqlServer` on Windows. Without one, jsonnet evaluation fails and the binary starts with an `{"error":…}` settings object. Test output is cwd-relative: logs go to `<cwd>/logs` and file-backed sqlite dbs to `<cwd>/sqlite-tests.db`.

Settings-related CLI flags (`libs/fwk/src/settings.cpp`):

| flag | effect |
|------|--------|
| `-settings=<file>` | the Jsonnet config to load |
| `-tests` / `-ctest` | test mode, as above; `addJdeTest` passes `-ctest` |
| `-include=<dir>[;<dir>…]` | replaces the import dirs, each relative to the settings file's directory. **Takes priority over the `-tests`/`-ctest` default** — but does not bind the ext vars, so pair it with a test flag |
| `-arg <k>=<v>` | binds jsonnet ext var `k`; split on the *first* `=`, so values may contain more |
| `-sync` | sets the `sync` top-level argument to `true`, enabling startup DDL schema-sync (off by default — see the `function( sync=false )` heading in the app configs) |

The three db-backed ctest suites — `libs/access/tests`, `apps/OpcGateway/tests`, and `apps/OpcServer/tests` — are wired to sqlite on **every** platform: their `addJdeTest` call adds `-include=args/sqlite -arg path=:memory:`, so ctest needs no db server and writes no db file. (A direct, non-ctest `-tests` run of the same binary still takes the default mysql/sqlServer import dir, since `-include` is only on the ctest registration.) The `libs/fwk` and `libs/web` suites use no database, and `libs/db/drivers/sqlite/tests` is inherently sqlite.

The two workflows use **different working directories**, so they keep separate logs/db files:

- **ctest**: `addJdeTest()` (`build/functions.cmake`) registers each suite with `WORKING_DIRECTORY $buildDir/Testing` (ctest creates it) and sets the `REPO_SOURCE_DIR`/`REPO_BUILD_DIR` env vars the configs expand via `$(…)` — output lands in `$buildDir/Testing/{logs,sqlite-tests.db}`.
- **direct/debugger runs**: run from `runtime/` (created by `reconfig`; the VS Code launch configs use it too) with `REPO_SOURCE_DIR`/`REPO_BUILD_DIR` exported in the shell — output lands in `$buildDir/runtime/{logs,sqlite-tests.db}`.

```bash
# Run a test binary directly
cd $buildDir/runtime
$buildDir/libs/fwk/tests/Jde.Fwk.Tests -tests -settings=$JDE_DIR/libs/fwk/tests/config/Framework.Tests.jsonnet

# Or every suite via ctest (addJdeTest passes -ctest and the env); --preset is broken, see reviews/todo.md §3
cd $buildDir && ctest
```

**Running a single test:** the `testing.tests` field in the Jsonnet config is the GoogleTest filter. Set it to e.g. `"FileTests.WriteRead"` (or a pattern like `"FileTests.*"`) to restrict the run. Some test binaries look for their config in `~/.Jde-Cpp/Tests.<Lib>/<Lib>.Tests.jsonnet` — symlink the repo config there if missing.

## Frontend (Angular)

The active Angular workspace is `web/opc/my-workspace/`. It is an Angular 22 workspace with one application (`my-workspace`) and four libraries (`jde-spa`, `jde-framework`, `jde-access`, `jde-opc`).

```bash
cd web/opc/my-workspace
ng serve          # dev server
ng build          # production build
ng test           # run tests (Vitest, not Karma)
ng build --watch --configuration development  # watch mode
```

The four libraries (`jde-spa`, `jde-framework`, `jde-access`, `jde-opc`) and the `my-workspace` application are all defined in `web/opc/my-workspace/angular.json`.

## C++ Code Conventions

### Macro aliases (defined in `include/jde/fwk/macros.h` and `usings.h`)

| Macro | Meaning |
|-------|---------|
| `α` | `auto` (member function return) |
| `β` | `virtual auto` |
| `Ω` | `static auto` |
| `Ξ` | `inline auto` |
| `ψ` | `template<class... Args> auto` |
| `ι` | `noexcept` |
| `Ι` | `const noexcept` |
| `ε` | `noexcept(false)` |
| `Γ` | dllexport/visibility("default") (from `exports.h`) |
| `Φ` | `Γ auto` (exported function) |
| `SL` | `std::source_location` |
| `SRCE_CUR` | `std::source_location::current()` |
| `FWD(a)` | `std::forward<decltype(a)>(a)` |

### Type aliases (from `usings.h`)

- `sv` = `std::string_view`, `str` = `const std::string&`
- `sp<T>` / `up<T>` / `wp<T>` = shared/unique/weak ptr
- `uint` = `uint_fast64_t`, `uint32` = `uint_fast32_t`, etc.
- `uuid` / `StringMd5` = `boost::uuids::uuid`
- `jvalue` / `jobject` / `jarray` = Boost.JSON types
- `flat_map`, `flat_set` = Boost.Container types

### Logging macros (from `log/log.h`)

Each file typically defines `_tags` as its `ELogTags` value. Then:

```cpp
INFO("message {}", value);       // ELogLevel::Information, uses _tags
WARN("message");
ERR("message");
DBG("message");
TRACE("message");
INFOT(tags, "message");          // explicit tags variant
```

### Stacktrace / compiler portability

Use `#ifdef __cpp_lib_stacktrace` to branch between `std::stacktrace_entry` (GCC/C++23) and `boost::stacktrace::frame` (clang). Do **not** use `#ifdef __clang__` for this purpose.

### Coroutines

Awaitables inherit from `VoidAwait` or `IAwait<TResult, TTask>` in `co/Await.h`. Tasks use `VoidTask` and typed task types from `co/Task.h`.

## Key Dependencies

- **fmt** + **spdlog** (external fmt, `SPDLOG_FMT_EXTERNAL=ON`)
- **Boost** (json, container, uuid, stacktrace, coroutine)
- **OpenSSL**
- **jsonnet** (config parsing)
- **open62541** (OPC-UA, OpcGateway/OpcServer only)
- **protobuf** + **abseil** (frontend/backend proto transport)
- **liburing** (Linux async I/O in fwk)

## Environment Variables

| Variable | Purpose |
|----------|---------|
| `$REPO_DIR` | Third-party/dependency root (e.g. `/home/duffyj/code/libs`) — **not** the jde repo. Presets install deps under `$REPO_DIR/install/$CXX/<buildType>`; `functions.cmake` reads Boost sources from `$REPO_DIR/boostorg` on Windows |
| `$JDE_DIR` | This checkout's source root (e.g. `/home/duffyj/code/jde/Public`); used by shell scripts and VS Code workspace configs — CMake files use paths relative to `CMAKE_CURRENT_LIST_DIR` instead |
| `$JDE_BASH` | Same as `$JDE_DIR` |
| `$JDE_BUILD_DIR` | Build-output parent directory (e.g. `/mnt/ram/linux`); actual build dirs are `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>/<debug\|release>` |
| `$JDE_COMPILER` | Compiler subdirectory name under `$JDE_BUILD_DIR` (e.g. `clang++`) |

## Other Stuff
- NEVER use compound Bash commands containing 'cd' and output redirection (e.g., cd dir && cmd > file).
- If you must execute a command in a different directory, always split it into two separate tool calls: first 'cd', then run the command.
- Prefer using absolute paths directly inside the command or tool parameters over chaining 'cd'.
- Never suggest enclosing single-line `if` statements with braces.
- Never run an unscoped/filesystem-wide search (e.g. `find /`, or any search rooted above the repo). Always scope file searches to `C:\Users\duffyj\source\repos` or `$JDE_BUILD_DIR` (build outputs, e.g. `x:\build` on Windows) — or a narrower subdirectory of either. A full-drive search can run for tens of minutes burning CPU for no benefit. Prefer Glob/Grep over shelling out to `find`/`grep` in the first place.
- GTest is installed (headers only, no vendored source) at `C:\Users\duffyj\source\repos\libs\install\<compiler>\<Debug|RelWithDebInfo>\gtest\include\gtest\` — look there directly instead of searching for it.

