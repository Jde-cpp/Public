# Jde.DB.Sqlite Driver Sketch

Embedded in-memory (SQLite) driver alongside the Odbc & MySql drivers.
Created 2026-07-08; building since 2026-07-10 with a passing `Jde.DB.Sqlite.Tests` target — see "Status" below.

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
| `SqliteProcs.h/.cpp` | Driver-internal proc registry (`RegisterProc`/`FindProc`/`RegisteredProcNames`) + the `ExecuteStatement`/`ScalarUInt` statement helpers. `Procs()` returns the `IProcs` (declared in `<jde/db/sqlite_api.h>`) the driver hands to each proc DLL's `RegisterProcs`, forwarding to these free functions — so a proc DLL registers + runs statements without linking the driver. `ProcΛ`/`IProcs` live in the public `sqlite_api.h`. |
| `SqliteDataSource.h/.cpp` | All `IDataSource` overrides. `Execute` dispatches `IsProc` statements to the registry wrapped in `begin immediate`/`commit`/`rollback`. `InsertSeqSyncUInt` just uses `sqlite3_last_insert_rowid` — no out-param dance. |
| `SqliteRow.h/.cpp` | `Bind`/`ToRow`/`ToValue` between `DB::Value` and `sqlite3_stmt`. |
| `SqliteQueryAwait.h` | Trivial vs MySql's asio coroutine: sqlite is in-process, so `Suspend()` executes synchronously and resumes immediately. Post to a worker pool if large scans ever block coroutine threads. |
| `SqliteServerMeta.h/.cpp` | Implemented via pragma table-valued functions (`pragma_table_info`, `pragma_index_list`+`pragma_index_info`, `pragma_foreign_key_list`) joined against `sqlite_master`. `LoadProcs` reports the native registry so DDL sync treats registered procs as existing. FK names aren't preserved by sqlite — synthesized as `<table>_fk<id>`; `SyncFKs` matches on Table+Columns, not name. `ToType` maps declared names first, then falls back to sqlite affinity rules. |
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
- **Declared types recover Time/Bool** (`SqliteRow::ToValue`): sqlite stores datetimes as epoch ints and
  bits as 0/1; `sqlite3_column_decltype` (contains `date`/`time` → Time, `bit`/`bool` → Bool) converts on
  read. Expressions have no decltype and stay ints.
- **Core `Syntax` knobs added for sqlite** (defaults preserve MySql/SqlServer behavior — access tests pass):
  - `HasProcs()` false → `View::InsertProcName` returns empty for generated procs, so `InsertClause::Move`
    takes the plain-`Insert` path and `SyncTables` skips `create procedure`. Custom procs
    (`HasCustomInsertProc`) keep their name and dispatch to the native registry.
  - `CanAddForeignKeys()` false → `SyncFKs` skips (`alter table add constraint` unsupported; FKs only
    enforced when inline in `create table`).
  - `SchemaExistsSql()` → `pragma_database_list`, so `main` always exists and `CREATE SCHEMA` never runs.
  - `ToString( EType )` now virtual: sqlite maps Int/UInt/Long/ULong → `integer` because the rowid alias
    (identity) requires the declared type be exactly `integer` — `int`/`bigint` pks don't auto-assign.

## Status (2026-07-10)

Done:
1. ~~sqlite headers~~ — static lib built from the 3.46.1 amalgamation into
   `$REPO_DIR/install/<compiler>/multi/sqlite` (clang++ & xg++-15); the driver/tests CMakeLists append that
   to `CMAKE_PREFIX_PATH` before `find_package( SQLite3 )`, so system `libsqlite3-dev` also works.
2. ~~Root `CMakeLists.txt`~~ — sqlite added outside the odbc/mysql platform conditional; tests under `jde_TESTS`.
3. ~~`SqliteServerMeta`~~ — implemented (see Files).
4. ~~DateTime round-trip~~ — decltype-driven (see above).
5. ~~Generated-proc paths~~ — `Syntax::HasProcs` (see above). Custom procs live in per-app
   `config/sql/sqlite/` dirs — one `<proc>.cpp` twin per `../mysql/<proc>.sql` (plus sqlite `.sql`
   translations of the views/triggers), built into per-app dlopen-able MODULEs whose single C export is
   `void RegisterProcs( Jde::DB::Sqlite::IProcs& )` (`include/jde/db/sqlite_api.h`). The driver implements
   `IProcs` (`RegisterProc` + the `ExecuteStatement`/`ScalarUInt` helpers) and hands it in; the DLLs register
   and run statements *through* it, so they **don't link the driver** — only `Jde.DB` (Value/Row) + a static
   sqlite3 (they call `last_insert_rowid` directly). The driver is still `SHARED`+dlopen'd; nothing links it now:
   - `Jde.AppServer.Sqlite` = `apps/AppServer/config/sql/sqlite/` + `libs/access/config/sql/sqlite/`
     (AppServer hosts access). The access user_insert procs `call` the *generated* `access_identity_insert`,
     which doesn't exist on sqlite — `AccessProcs::IdentityInsert` inlines its body.
   - `Jde.OpcGateway.Sqlite` = `apps/OpcGateway/config/sql/sqlite/`; the mysql file's trigger half became
     `gateway_server_connection_update.sql` (sqlite triggers can't assign NEW → `after update` + corrective
     update; own file because SyncScripts skips files named after registered procs).
   - `Jde.OpcServer.Sqlite` = `apps/OpcServer/config/sql/sqlite/` (7 procs + 5 view `.sql`s); mysql's
     `CRC32`/`BIN_TO_UUID` in the node-id computation are done in C++ (`boost::crc_32_type` = mysql CRC32).
6. ~~Tests~~ — `Jde.DB.Sqlite.Tests` (12 tests): round-trips incl. datetime/bit/blob/null, `returning`
   identity, native proc dispatch + rollback-on-throw, ServerMeta tables/indexes/fks/procs, dialect. Driver
   and AppServer-proc sources are compiled into the test target. First db driver testable in CI —
   no server needed.

Open:
- End-to-end `SyncSchema`/`GetAppSchema` integration test against `:memory:` (would exercise
  `TableDdl::CreateStatement` + data seeding through the sqlite dialect).
- `NonProd::Recreate`/`Drop` use `DROP SCHEMA` — meaningless for sqlite; a file-backed db recreates by
  deleting the file, `:memory:` by reconnecting.
- AppServer startup doesn't load `Jde.DB.Sqlite.AppServer` yet — dlopen it + call
  `RegisterProcs( Sqlite::Procs() )` when the configured driver is sqlite (config needs a way to point at the
  procs dll).
