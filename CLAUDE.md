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
- `web/opc/my-workspace/` — Angular 21 frontend (the active workspace)

## Building (C++)

Build outputs go to `/mnt/ram/`. The `$REPO_DIR` env var points to the repo root; `$JDE_DIR` points to the Public directory. Each compiler+checkout pair gets its own out-of-source build directory at `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>` (e.g. `/mnt/ram/linux/clang++/Public2` for this checkout). The `reconfig` function in `build/buildFunctions.sh` creates this layout; it also moves the generated `compile_commands.json` to the source root for clangd.

```bash
# Configure (from the matching out-of-source build dir)
cd $JDE_BUILD_DIR/$JDE_COMPILER/Public2   # e.g. /mnt/ram/linux/clang++/Public2
rm -f CMakeCache.txt
cmake /home/duffyj/code/jde/Public2 --preset linux-clang-debug-jde

# Or use the helper (creates the build dir and runtime/ if missing):
source build/buildFunctions.sh
reconfig $JDE_BUILD_DIR/$JDE_COMPILER /home/duffyj/code/jde/Public2 linux-clang-debug-jde

# Build a specific target (add -j$(nproc) for parallel builds)
cmake --build . --target Jde
cmake --build . --target Jde.Fwk.Tests

# Install
cmake --install .
```

Available CMake presets: `linux-debug`, `linux-relWithDebInfo`, `linux-jde-relWithDebInfo`, `linux-clang-debug-jde`, `win-msvc-debug`, `win-msvc-relWithDebInfo`, `win-clang-relWithDebInfo`.

Linux uses **g++-15** by default; `linux-clang-debug-jde` uses **clang++-22**. C++ standard is **C++26**.

## Running Tests (C++)

Tests use **GoogleTest**. Each test binary requires a `-settings=` argument pointing at a Jsonnet config. Per-library configs live at `libs/<lib>/tests/config/<Lib>.Tests.jsonnet` (and `apps/<app>/tests/config/<App>.Tests.jsonnet`):

- `libs/fwk/tests/config/Framework.Tests.jsonnet`
- `libs/db/drivers/sqlite/tests/config/Sqlite.Tests.jsonnet` (in-process, no db server needed)
- `libs/access/tests/config/Access.Tests.jsonnet`
- `libs/web/tests/config/Web.Tests.jsonnet`
- `apps/OpcGateway/tests/config/Opc.Tests.jsonnet`
- `apps/OpcServer/tests/config/Opc.Server.Tests.jsonnet`

```bash
# Run a test binary directly
./Jde.Fwk.Tests -settings=/home/duffyj/code/jde/Public2/libs/fwk/tests/config/Framework.Tests.jsonnet

# Via ctest (from build dir)
ctest --preset linux-debug

# Configure + build + run crypto/access/web tests in one shot
build/tests.sh            # Debug; args: <buildTarget> <clean> <buildDir>
```

**Running a single test:** the `testing.tests` field in the Jsonnet config is the GoogleTest filter. Set it to e.g. `"FileTests.WriteRead"` (or a pattern like `"FileTests.*"`) to restrict the run. `build/tests.sh` symlinks each config into `~/.Jde-Cpp/Tests.<Lib>/` for the binaries to find.

## Frontend (Angular)

The active Angular workspace is `web/opc/my-workspace3/`. It is an Angular 21 workspace with one application (`my-workspace`) and four libraries (`jde-spa`, `jde-framework`, `jde-access`, `jde-opc`).

```bash
cd web/opc/my-workspace3
ng serve          # dev server
ng build          # production build
ng test           # run tests (Vitest, not Karma)
ng build --watch --configuration development  # watch mode
```

The four libraries (`jde-spa`, `jde-framework`, `jde-access`, `jde-opc`) and the `my-workspace` application are all defined in `web/opc/my-workspace3/angular.json`. Older sibling workspaces (`my-workspace`, `my-workspace2`) are inactive — work only in `my-workspace3`.

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
| `$REPO_DIR` | Root of the repository |
| `$JDE_DIR` | `$REPO_DIR/Public` (source root; used by shell scripts and VS Code workspace configs — CMake files use paths relative to `CMAKE_CURRENT_LIST_DIR` instead) |
| `$JDE_BASH` | Same as `$JDE_DIR` |
| `$JDE_BUILD_DIR` | Build-output parent directory (e.g. `/mnt/ram/linux`); actual build dirs are `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>` |
| `$JDE_COMPILER` | Compiler subdirectory name under `$JDE_BUILD_DIR` (e.g. `clang++`) |

## Other Stuff
- NEVER use compound Bash commands containing 'cd' and output redirection (e.g., cd dir && cmd > file).
- If you must execute a command in a different directory, always split it into two separate tool calls: first 'cd', then run the command.
- Prefer using absolute paths directly inside the command or tool parameters over chaining 'cd'.
- Never suggest enclosing single-line `if` statements with braces.
- Never run an unscoped/filesystem-wide search (e.g. `find /`, or any search rooted above the repo). Always scope file searches to `C:\Users\duffyj\source\repos` or `$JDE_BUILD_DIR` (build outputs, e.g. `x:\build` on Windows) — or a narrower subdirectory of either. A full-drive search can run for tens of minutes burning CPU for no benefit. Prefer Glob/Grep over shelling out to `find`/`grep` in the first place.
- GTest is installed (headers only, no vendored source) at `C:\Users\duffyj\source\repos\libs\install\<compiler>\<Debug|RelWithDebInfo>\gtest\include\gtest\` — look there directly instead of searching for it.

