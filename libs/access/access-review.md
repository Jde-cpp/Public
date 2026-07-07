# Access Library Code Review

*Reviewed 2026-07-07 (branch `fwk`, clean tree at 1168a6c). Full read-through of `libs/access` (~2,800 lines of source), public headers in `include/jde/access`, tests, SQL, and CMake.*

## Overview

Compact, coherently layered RBAC/ACL library — in-memory `Authorize` cache, QL-driven loaders, event listener to keep it in sync, and server-side awaits for mutations. The lock-token pattern (passing the lock as a parameter to prove it's held) is nice. Several real correctness bugs exist in the permission-recalculation logic, plus one authorization gap.

## High severity

### 1. Revoking an ACL entry doesn't take effect in memory — `RemoveAcl` never clears the user
`src/Authorize.cpp:233-247`: `RemoveAcl` erases the entry from `Acl`, then calls `SetUserPermissions({user}, l)`. But `SetUserPermissions`/`AddPermission` are purely additive (`User::operator+=` ORs into `Rights`), so the revoked permission stays in `user.Rights` until an unrelated full `Recalc`. Everywhere else that calls `SetUserPermissions` clears the affected users first (`Recalc`, `AddToGroup`, `RemoveFromGroup`); this path doesn't. A purged ACL grant remains effective until restart.

### 2. `RecursiveUsers`'s `clear` flag is inverted
`src/Authorize.cpp:134`: `auto user = clear ? Users.end() : Users.find(member.UserPK())` — when `clear` is **true** it skips `Clear()`, and when false it clears. Every caller about to recalculate (`RecalcGroupMembers`, `AddToGroup`, `RemoveFromGroup`) passes `clear=true`, so direct user members are *not* cleared — while nested groups' users *are* (the inner recursion at line 139 uses the default `false`). Consequence: deleting/purging a group, or removing a nested group from a group, leaves its direct user members with stale rights (same additive-only problem as #1), and behavior differs by nesting depth.

### 3. `createAcl` with a role has no authorization check — privilege escalation
`src/server/awaits/AclAwait.cpp:73-89`: `InsertRole` inserts the identity→role ACL row with no `TestAdmin` call. Compare its siblings: `InsertPermission` checks `TestAdmin(resourcePK, _executer)` (line 117), and the *purge* of a role ACL checks `TestAdmin("roles", _executer)` (line 33). As written, any authenticated user who can reach the `acl` create mutation can grant any role (including admin roles) to any identity — including themselves. Tests never exercise this path with a non-root executer.

### 4. `User::UpdatePermission` corrupts sibling permissions and drops rights
`src/types/User.cpp:7-24`: the loop applies the *update* (`p.Update(allowed, denied)`) to **every** permission sharing the resource, not just `permissionPK`, then recomputes `Rights[resourcePK]` only from the new `*allowed`/`*denied` values — ignoring the actual accumulated rights of the other permissions. If only `denied` is passed, `rights.Allowed` stays `None` and the user's allowed rights for that resource are wiped. Fix: update only the target permission, then re-OR `p.Allowed`/`p.Denied` from each sibling.

## Medium severity

### 5. `FindResource` looks up the wrong PK
`src/Authorize.cpp:31`: after resolving `pk` from schema/target, the lookup is `Resources.find(resource.PK)` instead of `find(*pk)`. When the caller passes a resource with `PK==0` but schema+target set (as `AddRolePermission` does from listener events), the find is `find(0)` → miss → falls through and logs `CRITICAL`/drops the role permission even though the resource exists.

### 6. ACL purge event never removes the permission from memory (verify at runtime)
`src/AccessListener.cpp:145`: the `Purged` handler checks `o.if_contains("permission")`, but everything producing the payload uses the key `"permissionRight"` — the Created handler (line 130), the subscription query (`src/awaits/EventsSubscribeAwait.cpp:54`), and `PurgeAcl`'s mutation args. Unless the QL layer renames the key, purging a permission-type ACL leaves it active in memory (the role path works because it checks `"role"`).

### 7. `Role` parses `deleted` as a bool; every other loader treats it as a timestamp (verify at runtime)
`src/types/Role.cpp:21` uses `Json::FindBool(j,"deleted").value_or(false)`, while `IdentityLoadAwait` and `Resource` use `FindTimePoint`. If the roles table's `deleted` column is a datetime like the others, deleted roles load as *active*.

### 8. ACL reads bypass authorization
`AclQLSelectAwait` (`src/server/awaits/AclAwait.cpp:136-280`) queries via `DS().SelectAsync` directly with no rights check on `_executer` (stored but never used). `GroupAwait::Select` by contrast calls `GetTable("groupings").Authorize(Read, _executer)` first. Any user can enumerate the full ACL — identities, roles, and who holds what rights — useful reconnaissance for #3.

## Minor

- **`TestAdminPermission`** (`src/Authorize.cpp:94-99`): `l.unlock()` then dereferences the `Permissions` iterator (`permission->second.ResourcePK`) — a concurrent erase invalidates it. Read the ResourcePK into a local before unlocking.
- **Noexcept + throwing JSON accessors**: `AclChanged` is `ι` (noexcept) but calls `Json::AsNumber` which throws → `std::terminate` on a malformed event payload. Several listener methods have this shape; `OnChange` is `ε`, so catch/log there or use `Find…` accessors.
- **`Resource` default ctor** leaves `PK` (uint16) uninitialized (`include/jde/access/types/Resource.h:12`).
- **Cycle guards only at write time**: `RecursiveUsers` and `AddPermission`'s role recursion have no runtime cycle protection; a cycle already in the DB (or introduced through the raw SQL path) means unbounded recursion. `TestAddGroupMember`/`TestAddRoleMember` only guard new inserts.
- **Header/impl drift**: `RemoveRoleChildren` is declared with `flat_set<RolePK>` but defined with `flat_set<PermissionPK>` (same alias, so it compiles); `PermissionRightPK` in `include/jde/access/types/Role.h:6` duplicates `PermissionRightsPK` in `usings.h`. Also `TestAdmin(str resourceTarget,…)` matches the first resource with that target across *all* schemas — its sibling `GetSchema` bothers to exclude dotted OPC schemas, this doesn't.
- **Load-then-subscribe gap**: `ConfigureAwait` loads users → resources → roles → ACL and subscribes *last*, so mutations landing during startup are silently missed.
- **CMake header GLOB** misses `include/jde/access/server/**`, `awaits/`, and `client/` headers (cosmetic — IDE only).

## Design observations (intentional, but worth stating)

- **Fail-open by design**: if a resource isn't registered/enabled, `Test` passes and `Rights` returns `All` (the `DisabledPermissions` test confirms this is deliberate). A deleted `resources` row disables *all* protection for that target — a fragile default for an access-control system; a config flag to fail closed would be cheap insurance.
- **Auto-provisioning**: `AuthenticateAwait` creates a user for any unknown login name — fine assuming OAuth token validation happens upstream (in the web layer), but this library itself never verifies anything; `Authenticate(loginName, providerId)` is authentication-by-assertion, so every caller must be trusted.
- **Tests are integration-only** (live DB, root executer for most setup). Coverage of allow/deny/hierarchy flows is decent, but there are no unit tests for the in-memory `Authorize` recalculation logic — exactly where bugs #1, #2, #4, and #5 live. That class is testable in isolation and would benefit most.

## Suggested priority

1. #1/#2/#4 — in-memory revocation correctness (one cluster, likely one fix session)
2. #3 — add `TestAdmin` to `InsertRole`
3. #5–#8
