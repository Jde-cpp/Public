# Jde.DB.Sqlite Driver Sketch

Skeleton for an embedded in-memory (SQLite) driver alongside the Odbc & MySql drivers.
Created 2026-07-08 on branch `opcGateway-refactor`; not yet compiled — see "To take it from sketch to building" below.

## Why SQLite (vs DuckDB / CrossDB / ObjectBox)

- **SQLite** — embedded row-store OLTP engine, `:memory:` mode, battle-tested, single ~1 MB amalgamation,
  public domain. Dialect slots cleanly into the `Syntax` pattern. Matches the workload: small transactional
  inserts/upserts with identity retrieval (app registry, users, ACLs).
- **DuckDB** — columnar/OLAP; point inserts/updates are expensive. Only wins for in-process analytics over
  OPC historical data — a different feature.
- **CrossDB** — OLTP-shaped but young, effectively single-maintainer, limited dialect. Maturity risk not
  worth it for a security/config store.
- **ObjectBox** — not SQL; the entire `generators/` layer, `Syntax` dialects, and `.sql` proc files would be
  dead weight.

## The stored-procedure gap

SQLite has no server, hence no server-side procs. Two approaches, combined here:

1. **Generate plain SQL instead of procs.** `InsertClause` already has an `IsStoredProc` flag; a SQLite
   dialect can route insert/upsert generators down the non-proc path — `INSERT ... RETURNING` replaces
   `call <table>_insert(...)` + out-param; upserts become `INSERT ... ON CONFLICT DO UPDATE`.
2. **Register hand-written procs as native C++.** Each `config/sql/{sqlServer,mysql}/<name>.sql` proc gets a
   C++ twin registered under the same name (`SqliteProcs.h`). The data source intercepts `Sql::IsProc` and
   dispatches inside a transaction. Callers are unaware; out-params come back as a single result row — the
   same shape MySql's `out_params()` produces.

## Files

| File | Purpose |
|------|---------|
| `SqliteSyntax.h` | Driver-local dialect. No core changes needed — syntax resolves dynamically through `IDataSource::Syntax()`. Could move next to `MySqlSyntax` in `include/jde/db/generators/Syntax.h` if generators ever need it statically. |
| `SqliteProcs.h/.cpp` | Proc registry (`ProcΛ`: name → native function) + statement helpers. Includes a worked example: the native twin of `app_instance_insert` (find-or-create host, insert instance, return `_instanceId`). Registration belongs in each app's startup. |
| `SqliteDataSource.h/.cpp` | All `IDataSource` overrides. `Execute` dispatches `IsProc` statements to the registry wrapped in `begin immediate`/`commit`/`rollback`. `InsertSeqSyncUInt` just uses `sqlite3_last_insert_rowid` — no out-param dance. |
| `SqliteRow.h/.cpp` | `Bind`/`ToRow`/`ToValue` between `DB::Value` and `sqlite3_stmt`. |
| `SqliteQueryAwait.h` | Trivial vs MySql's asio coroutine: sqlite is in-process, so `Suspend()` executes synchronously and resumes immediately. Post to a worker pool if large scans ever block coroutine threads. |
| `SqliteServerMeta.h/.cpp` | Stubbed with `THROW`s; each method documents its pragma (`table_info`, `index_list`+`index_info`, `foreign_key_list`). `LoadProcs` should report the native registry so DDL sync treats registered procs as existing. |
| `exports.h` / `usings.h` / `pc.h` / `CMakeLists.txt` | Boilerplate mirroring the mysql driver; links `SQLite::SQLite3`. |

## Design decisions

- **Single connection behind `_connMutex`**, not a MySql-style session pool: an in-memory database is
  per-connection — a pool would hand each caller its own empty database. File-backed dbs get
  `pragma journal_mode=wal` and could pool later.
- **Config**: `{ "driver": ".../Jde.DB.Sqlite.so", "path": ":memory:" | "/var/lib/jde/gateway.db" }`.
- **Dialect choices** (`SqliteSyntax.h`): `last_insert_rowid()` identity; `blob` GUIDs; datetimes stored as
  unix-epoch integers (`unixepoch()` for `UtcNow`/`NowDefault`, must match `SqliteRow::Bind`);
  `limit/offset` paging; no catalogs; schema is always `main`; identity column relies on the
  `integer primary key` rowid alias (see `CreatePrimaryKey`).
- **Out params**: MySql passes them as trailing placeholders; the native proc returns them as a row instead,
  so `ExecuteProc` drops the trailing placeholder before dispatch.

## To take it from sketch to building

1. `sudo apt install libsqlite3-dev` — until then every `.cpp` shows false "sqlite3.h not found" errors
   (only runtime `libsqlite3-0` is installed).
2. Add `add_subdirectory( ${libDir}/db/drivers/sqlite libs/db/drivers/sqlite )` to the root
   `CMakeLists.txt` (~line 31–35). It currently splits odbc (WIN32) / mysql (else); sqlite builds on both
   platforms, so it likely belongs outside the conditional.
3. Implement `SqliteServerMeta` — needed for DDL sync / `SyncSchema`.
4. Decide the `DateTime` round-trip: `ToValue` can't distinguish an epoch integer from a plain int without
   schema-driven column types (TODO at `SqliteRow.cpp` `ToValue`).
5. Register each app's native procs at startup (see `RegisterAppServerProcs` for the shape) and verify the
   generated-proc paths (`InsertClause::Proc`) either dispatch to the registry or switch to the plain-SQL
   generator path.
6. Tests: a `Jde.DB.Sqlite.Tests` target would be the first db driver with in-process tests — no server
   needed, so it could run in CI.
