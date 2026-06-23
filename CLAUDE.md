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
- `web/opc/my-workspace3/` — Angular 21 frontend (the active workspace)

## Building (C++)

Build outputs go to `/mnt/ram/`. The `$REPO_DIR` env var points to the repo root; `$JDE_DIR` points to the Public directory. Each preset gets its own out-of-source build directory under `/mnt/ram/linux/` (e.g. `clang++-22` for `linux-clang-jde-debug`, `g++-15` for `linux-debug`).

```bash
# Configure (from the matching out-of-source build dir)
cd /mnt/ram/linux/clang++-22      # or /mnt/ram/linux/g++-15 for linux-debug
rm -f CMakeCache.txt
cmake /home/duffyj/code/jde/Public --preset linux-clang-jde-debug

# Build a specific target (add -j$(nproc) for parallel builds)
cmake --build . --target Jde
cmake --build . --target Jde.Fwk.Tests

# Install
cmake --install .
```

Available CMake presets: `linux-debug`, `linux-relWithDebInfo`, `linux-jde-relWithDebInfo`, `linux-clang-jde-debug`, `win-msvc-debug`, `win-msvc-relWithDebInfo`, `win-clang-relWithDebInfo`.

Linux uses **g++-15** by default; `linux-clang-jde-debug` uses **clang++-22**. C++ standard is **C++26**.

## Running Tests (C++)

Tests use **GoogleTest**. Each test binary requires a `-settings=` argument pointing at a Jsonnet config. Per-library configs live at `libs/<lib>/tests/config/<Lib>.Tests.jsonnet` (and `apps/<app>/tests/config/<App>.Tests.jsonnet`):

- `libs/fwk/tests/config/Framework.Tests.jsonnet`
- `libs/access/tests/config/Access.Tests.jsonnet`
- `libs/web/tests/config/Web.Tests.jsonnet`
- `apps/OpcGateway/tests/config/Opc.Tests.jsonnet`
- `apps/OpcServer/tests/config/Opc.Server.Tests.jsonnet`

```bash
# Run a test binary directly
./Jde.Fwk.Tests -settings=/home/duffyj/code/jde/Public/libs/fwk/tests/config/Framework.Tests.jsonnet

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
| `$JDE_DIR` | `$REPO_DIR/Public` (source root) |
| `$JDE_BASH` | Same as `$JDE_DIR` (used in some CMake files) |
| `$JDE_BUILD_DIR` | Active build output directory (e.g. `/mnt/ram/linux/clang++-22`) |

## Other Stuff
- NEVER use compound Bash commands containing 'cd' and output redirection (e.g., cd dir && cmd > file).
- If you must execute a command in a different directory, always split it into two separate tool calls: first 'cd', then run the command.
- Prefer using absolute paths directly inside the command or tool parameters over chaining 'cd'.
- Never suggest enclosing single-line `if` statements with braces.

