# libs/opc Review — 2026-07-07

Scope: `libs/opc/src` + `include/jde/opc` (the Jde.Opc library only; OpcGateway/OpcServer apps not reviewed, though a few call sites were checked to confirm severity). All line numbers as of branch `db-refactor`.

## Critical

### 1. `Variant::operator=(Variant&&)` is a no-op that destroys the source
`src/uatypes/Variant.cpp:85` — `UA_Variant{ move(v) };` constructs a *temporary* and discards it; `*this`'s base is never assigned. The next line `UA_Variant_init(&v)` then wipes the source. Net effect: the target keeps its old value, the source's heap data is leaked, and only `VariantPK` transfers. Every move-assignment silently loses the value.

Fix: clear `*this`, shallow-copy the base (`static_cast<UA_Variant&>(*this) = v;`), then init the source — same shape as `Value::operator=(Value&&)` in `Value.cpp:12`.

### 2. `Variant` has no destructor — every owning Variant leaks
`include/jde/opc/uatypes/Variant.h` — `Variant` owns heap data (`UA_Variant_setScalarCopy`, `UA_Variant_copy`, `ToUAValues`) but declares no destructor, so `UA_Variant_clear` is never called. All sibling wrappers (`NodeId`, `Value`, `BrowseName`, `LocalizedText`, `UAString`, `ExNodeId`) have clearing destructors; this one was missed. Long-running OpcServer/OpcGateway processes leak on every Variant constructed from JSON or DB.

(Adding the destructor makes finding #4 a crash instead of a leak — fix them together.)

### 3. Heap buffer overflow in `Variant::ToUAValues` array path
`src/uatypes/Variant.cpp:172-175` — for `size>1`, `UA_Array_new(size, &type)` allocates `size × type->memSize` bytes, but the loop stores `size` **pointers**: `((void**)data)[i] = GetUAValue(...)`. For any type with `memSize < sizeof(void*)` (Boolean=1, Int16=2, Float=4, UInt32=4 …) this writes past the allocation — heap corruption. Used from `OpcServer/src/awaits/VariantAwait.cpp:53` and `VariableAwait.cpp:40` with DB-driven types.

Even when `memSize == 8`, the result is an array of pointers where `UA_Variant` (storageType `UA_VARIANT_DATA`) is contracted to hold a contiguous array of values — `UA_Variant_clear`, `UA_encodeJson`, or handing the variant to the open62541 server reads garbage. The `catch` cleanup (`UA_Array_delete(data, size, &type)`) likewise interprets pointer slots as structs and leaks every `GetUAValue` allocation.

Fix: decode each element into `((UA_Byte*)data) + i*type.memSize` via `UA_decodeJson` directly (then the JSON path in `ToUAJson`'s array branch must also switch from `((void**)data)[i]` to `data + i*memSize`).

### 4. `Variant(StatusCode)` smuggles the code through the data pointer → wild dereference
`src/uatypes/Variant.cpp:70-73` — the status code is cast into `data` (`(void*)(uintptr_t)sc`). `ToJson` compensates (`toJson(&data, *type)` at line 148-149), but `ToUAJson()` (line 120) passes the fake pointer straight to `UA_encodeJson` → dereference of e.g. `0x80330000`. And once #2 is fixed, `UA_Variant_clear` would free it (`UA_VARIANT_DATA_NODELETE` guards that today, but it's fragile). Store a real scalar via `UA_Variant_setScalarCopy` instead.

### 5. Malformed GUID from a client terminates the process
`include/jde/opc/uatypes/opcHelpers.h:24` — `ToGuid(string, UA_Guid&)` is `ι` (noexcept) but `boost::lexical_cast<uuid>` throws on invalid input. Reached from `NodeId::FromJson` (`NodeId.cpp:117`) and `ExNodeId`'s rest-params ctor (`ExNodeId.cpp:40`), both of which parse client-supplied QL/REST JSON. A single request with `{"g":"not-a-guid"}` → exception escapes a noexcept function → `std::terminate` → remote DoS of the gateway. Make it `ε` (or `TryTo`-style returning `optional`) and validate.

### 6. `NodeId(DB::Row)` / `ExNodeId(DB::Row)` leave the base uninitialized
`src/uatypes/NodeId.cpp:25`, `src/uatypes/ExNodeId.cpp:62` — neither ctor initializes its POD base (`UA_NodeId{}` / `UA_EXPANDEDNODEID_NULL`), so `identifierType`/`identifier` start indeterminate. If all four identifier columns are NULL (or only set later), the object carries garbage; the virtual destructor then runs `UA_NodeId_clear` on a wild pointer → crash or arbitrary free. Add the base initializer (every other ctor in these classes has one).

## High

### 7. `ExNodeId` move-assign and `SetNodeId` leak the target's old resources
`src/uatypes/ExNodeId.cpp:111-117` — `operator=(ExNodeId&&)` overwrites `nodeId` and `namespaceUri` without calling `Clear()` first (the copy-assign at line 128 does). Any string/bytestring identifier or namespace URI previously held is leaked.
`src/uatypes/ExNodeId.cpp:102-109` — `SetNodeId` clears `namespaceUri` but not the old `nodeId`'s string/bytestring identifier before overwriting → same leak.

### 8. `ExNodeId::Copy()` corrupts byte-string identifiers containing NUL
`src/uatypes/ExNodeId.cpp:161` — `UA_BYTESTRING_ALLOC(string{ToSV(...)}.c_str())` measures with `strlen`, so a binary identifier containing `0x00` is truncated at the first NUL. Copy ctor and copy-assign both route through `Copy()`, so copying such a node silently changes its identity. Use `UA_ByteString_copy`. (Same `c_str()` pattern for `namespaceUri` at lines 13/132 is harmless for URIs but sloppy; `AllocUAString(str)` in `UAString.h:9` has the same truncation property.)

### 9. Variant copy-assign leaks the target
`src/uatypes/Variant.cpp:75-80` — `UA_Variant_copy(&v, this)` does not free the destination's existing contents (open62541's `UA_copy` assumes an uninitialized dst). Call `UA_Variant_clear(this)` first. Same pattern in `NodeId::operator=(const NodeId&)` is done correctly (`NodeId.cpp:62`), so this is just an omission here. `Value::Set` (`Value.cpp:83`) has the sibling problem: repeated `Set` on the same `Value` calls `UA_Variant_setScalarCopy` over an already-populated `value` → leak per call.

## Medium

### 10. Move-from-raw-UA-type ctors don't null the source
`include/jde/opc/uatypes/ExNodeId.h:12` (`ExNodeId(UA_NodeId&&)`) and `BrowseName.cpp:18` (`BrowseName(UA_QualifiedName&&)`) copy the pointers but never `_init` the source — unlike `Value(UA_DataValue&&)`, `NodeId(UA_NodeId&&)`, `Variant(UA_Variant&&)`, which do. Result is double ownership: if the caller (or a `NodeId`/`BrowseName` temporary whose dtor clears) also releases the source, the new object dangles / double-frees. Make all six consistent.

### 11. GUID byte order doesn't match RFC 4122 text form
`opcHelpers.h:22-25` — `ToJson(UA_Guid)`/`ToGuid` `memcpy` between `UA_Guid` (native-endian `data1/2/3` fields) and `boost::uuids::uuid` (big-endian byte array). On little-endian machines the JSON/DB text of a GUID node id has the first three groups byte-swapped relative to what the OPC server and every other UA tool display. Round-trips inside Jde are self-consistent, so this survives testing — but stored/displayed GUIDs won't match external tooling, and `ToGuid(string)` will address the *wrong node* when a user pastes a GUID from a UA browser. Swap `data1/2/3` explicitly (cf. `UA_Guid` ↔ RFC conversion).

### 12. `UADateTime(jvalue)` rejects its own `ToJson()` output
`src/uatypes/DateTime.cpp:25` — the object branch requires `"seconds"` to itself be an object (protobufjs `Long` form `{high,low}`), but `UADateTime::ToJson()` (line 45-48) emits plain numbers, and `Value::Set`'s duration path reads plain numbers too. So `Value::Set` on a DATETIME node with `{"seconds":123,"nanos":0}` throws "Invalid DateTime object". Accept numeric `seconds` as well.

### 13. Dead, ill-formed request headers
`src/uatypes/CreateSubscriptionRequest.h`, `src/uatypes/CreateMonitoredItemRequest.h` — included nowhere in the repo, and their mem-init lists initialize *inherited* members (`requestedPublishingInterval{...}`, `itemToMonitor{...}`), which is ill-formed — they'd fail to compile if ever included. `CreateMonitoredItemRequest` additionally takes `NodeId` by value and shallow-copies it into `itemToMonitor`; the param's destructor would free the identifier at end of ctor → dangling request. Delete them, or convert to `Default()` factory functions that assign in the body.

### 14. Noexcept ctors wrapping throwing JSON accessors
`BrowseName(const jobject&)` (`BrowseName.cpp:5`, `ι`) calls `Json::AsString(j,"name")`, which throws when `"name"` is missing/non-string → `std::terminate` on malformed input. `Variant(const jvalue&, sv)` (`Variant.cpp:23`, `ι`) calls `Json::AsNumber<double>` similarly at line 34. Either mark `ε` or use the `Find*` variants with defaults.

## Low / cleanup

- **Unchecked base64 decodes** — `ExNodeId.cpp:36`, `NodeId.cpp:113`: `UA_ByteString_fromBase64` status ignored; invalid input silently yields an empty/partial identifier. Also unchecked: every `UA_Variant_setScalarCopy`, `UA_NodeId_copy`, `UA_ByteString_allocBuffer` in the library (OOM → silently empty object).
- **Exception-path leaks in `ε` ctors** — `ExNodeId(flat_map)` (`ExNodeId.cpp:17`): if `stoul` throws after `namespaceUri` was allocated, the destructor doesn't run and the string leaks. `stoul` also accepts/throws inconsistently vs the `Str::TryTo` used for `"ns"` — use `TryTo` for `"serverindex"`/`"i"` too.
- **`ToUAByteString( const T&& bytes )`** (`opcHelpers.h:28`): `const T&&` binds only const rvalues — almost certainly meant `const T&` or a forwarding ref; alloc result unchecked before `memcpy`.
- **Missing `{}` in log format** — `Variant.cpp:20,38,43`: `WARNT(..., "Unsupported data type in Variant: ", serialize(v))` — no placeholder, so the serialized value never appears in the log.
- **`Variant(jvalue)` type support** — only bool/string/double are handled; integer JSON values fall through to a warn + null-string scalar. At minimum handle int64/uint64 like `Value::Set` does.
- **`getDataType()`** (`Variant.cpp:10`) appears unused — dead code (and duplicates the ctor's logic).
- **`FindDataType`** (`opcHelpers.cpp:5`) deep-copies a `NodeId` per iteration over all `UA_TYPES_COUNT` types; `UA_NodeId_equal(&UA_TYPES[i].typeId, &nodeId)` avoids ~145 allocations per lookup.
- **`exports.h`** — `ΓOPC` is defined empty with the real dllexport machinery commented out; fine while the lib is static, but the `ΓOPC` annotations are currently decorative.
- **Slicing hazard by design** — the wrappers convert implicitly to their UA bases while owning heap data (e.g. `(UA_ExpandedNodeId)ExNodeId{...}` in `OpcServer/src/UAServer.cpp:143` — safe there only because the temporary outlives the full expression). Worth a comment or `explicit` conversion helpers; it's the same class of trap as #10.

## Suggested fix order
1. #1 + #2 + #9 (Variant assignment/destructor — one small patch, biggest payoff)
2. #5 (remote terminate) and #6 (uninitialized base)
3. #3 (array representation — needs a matching change in `ToUAJson`)
4. #7, #8, #10 (ExNodeId/BrowseName ownership)
5. The rest as cleanup; delete the dead request headers (#13).

Tests: the library has no tests target of its own (`libs/opc/src` only). #1, #3, #8, #12 are all easily unit-testable — a small `Jde.Opc.Tests` covering Variant round-trips (scalar + array, all numeric types), ExNodeId copy/move with string & bytestring ids, and DateTime JSON round-trip would have caught most of this list.
