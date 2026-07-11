#pragma once
#include "../../../../db/drivers/sqlite/src/SqliteProcs.h"

struct sqlite3;

//Native twins of the server procs in the sibling mysql/sqlServer dirs - one <proc>.cpp per proc, same names,
//same param order, out params returned as the result row. Registered with the AppServer procs by
//RegisterAppServerProcs (jde/db/sqlite_api.h) in apps/AppServer/config/sql/sqlite.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessAcInsertRole()ι->void;
	α RegisterAccessAcUpsertPermission()ι->void;
	α RegisterAccessProviderPurge()ι->void;
	α RegisterAccessRoleAdd()ι->void;
	α RegisterAccessRoleInsert()ι->void;
	α RegisterAccessRolePurge()ι->void;
	α RegisterAccessRoleRemove()ι->void;
	α RegisterAccessUserInsert()ι->void;
	α RegisterAccessUserInsertKey()ι->void;
	α RegisterAccessUserInsertLogin()ι->void;

	//Twin of the *generated* access_identity_insert proc the mysql user_insert procs `call` - sqlite has no
	//generated procs (Syntax::HasProcs false), so its body is inlined here: insertable columns + created=$now.
	α IdentityInsert( sqlite3& db, const Value& name, const Value& providerId, const Value& target, const Value& attributes, const Value& description, const Value& isGroup, SL sl )ε->uint; //returns new identity_id.
}
