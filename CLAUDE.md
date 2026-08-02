# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a monorepo for the **Jde OpcGateway** system — an OPC-UA gateway with REST/WebSocket API, Angular frontend, and C++ backend services.

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

Linux uses **clang++-22** with libc++ (plus ASan/LSan in debug); Windows uses clang too. C++ standard is **C++26**. Every selectable configure preset (`cmake --list-presets`) carries one of two suffixes; everything else is a `hidden` building block.

The presets are **split by OS across four files** (presets schema `version: 9`). The auto-loaded `CMakePresets.json` only does `"include": ["CMakePresets.${hostSystemName}.json"]`, which resolves to `CMakePresets.Windows.json` or `CMakePresets.Linux.json` — capitalized to match `CMAKE_HOST_SYSTEM_NAME`, and case must match exactly since Linux is case-sensitive. Each OS file in turn includes `CMakePresets.common.json`, which holds the OS-agnostic hidden building blocks (`common`, `repos`, `debug`, `relWithDebInfo`, `release`, `clang`, `clang-jde`). So on any host `cmake --list-presets` shows only that OS's presets. `common.json` **must stay a separate file**: a preset may only inherit from its own file or a file it includes (inheritance flows downward along includes), so the shared blocks can't be folded up into the root — the OS files include the common file and inherit from it. `${hostSystemName}` is allowed in `include` because it is not a preset-specific macro (unlike `${presetName}`/`${generator}`), but that needs schema `version: 9`.

**`-jde`** — builds the project (libs, apps, tests: `jde_TESTS=ON`, `jde_APPS=ON`), adding `CMAKE_EXPORT_COMPILE_COMMANDS` and the house warning exclusions. This is the day-to-day dev build:

| compiler | debug | relWithDebInfo |
|---|---|---|
| clang | `linux-clang-debug-jde` | `linux-clang-relWithDebInfo-jde` |
| win clang | `win-clang-debug-jde` | — |

**`-repos`** — builds only the third-party dependencies (`jde_REPOS=ON`, `jde_TESTS=OFF`, `jde_APPS=OFF`):

| compiler | debug | relWithDebInfo | release |
|---|---|---|---|
| clang | `linux-clang-debug-repos` | `linux-clang-relWithDebInfo-repos` | — |
| win clang | `win-clang-debug-repos` | — | `win-clang-release-repos` |
| g++ | `linux-debug-repos` | `linux-relWithDebInfo-repos` | — |

The g++ (`linux-*`, **g++-15**) presets exist only for the dependency build — there is no g++ `-jde` preset, so the project itself is built with clang. Note the leftover `linux-debug` **build**/**test** presets are currently unusable: they name the now-hidden `linux-debug` configure preset, and CMake rejects that (`Cannot use hidden configure preset`) — see `reviews/todo.md` §3.

## Running Tests (C++)

See the **`cpp-tests`** skill (`.claude/skills/cpp-tests/SKILL.md`) for the full procedure — the required `-settings` and `-tests`/`-ctest` flags, the settings CLI flag table, which suites are sqlite-backed under ctest, the differing working directories for ctest vs direct runs, and single-test filtering.

## Frontend (Angular)

The active Angular workspace is `web/opc/my-workspace/` — Angular 22, one application (`my-workspace`) and four libraries (`jde-spa`, `jde-framework`, `jde-access`, `jde-opc`), all defined in its `angular.json`. Standard `ng` commands apply from that directory; note `ng test` runs **Vitest, not Karma**.

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
- Markdown docs that cite repo files (reviews, findings, design notes) must link them per the **`md-file-refs`** skill — `` [`DBException.h:37`](../Public/include/jde/db/DBException.h#L37) ``, never `file://` (the VS Code preview blocks it) or root-relative. Run its `linkify.py` over the doc when finishing it; no need to ask first.

