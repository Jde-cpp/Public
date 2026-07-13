#pragma once
#include <jde/db/generators/Syntax.h>

namespace Jde::DB::Sqlite{
	//Driver-local dialect - resolved dynamically through IDataSource::Syntax(), so nothing in Jde.DB needs to know about it.
	//Could move next to MySqlSyntax in include/jde/db/generators/Syntax.h if DDL/generators ever need it statically.
	struct SqliteSyntax final : Syntax{
		Ω Instance()->const SqliteSyntax&{ static const SqliteSyntax _instance; return _instance; }
		α AltDelimiter()Ι->sv override{ return {}; }
		α CanAddForeignKeys()Ι->bool override{ return false; } //no 'alter table add constraint' - fks only enforced when inline in create table.
		α CanSetDefaultSchema()Ι->bool override{ return false; } //single db per connection; ATTACH could emulate schemas.
		α CatalogSelect()Ι->sv override{ return {}; }
		α DateTimeSelect( sv columnName )Ι->string override{ return string{columnName}; } //stored as unix epoch integer - see SqliteRow.
		α DriverReturnsLastInsertId()Ι->bool override{ return true; } //sqlite3_last_insert_rowid.
		α EscapeDdl( sv sql )Ι->string override{ return Ƒ( "\"{}\"", sql ); } //TODO: multi-part names like base impl.
		α GuidType()Ι->sv override{ return "blob"; }
		α HasLength( EType )Ι->bool override{ return false; } //type affinity - lengths are documentation only.
		α HasCatalogs()Ι->bool override{ return false; }
		α HasProcs()Ι->bool override{ return false; } //generated insert procs -> plain sql + last_insert_rowid; hand-written procs dispatch to SqliteProcs registry.
		α HasSchemas()Ι->bool override{ return false; }
		α HasUnsigned()Ι->bool override{ return false; }
		α IdentityColumnSyntax()Ι->sv override{ return {}; } //rowid alias: pk must be declared 'integer primary key' - see CreatePrimaryKey.
		α IdentitySelect()Ι->sv override{ return "last_insert_rowid()"; }
		α CreatePrimaryKey( str /*tableName*/, str columns )Ι->string override{ return Ƒ("PRIMARY KEY( {} )", columns); } //columns: comma-separated for composite keys. Single-column integer pk stays a rowid alias.
		α Limit( str sql, uint limit, uint skip )Ι->string override{ return Ƒ("{} limit {} offset {}", sql, limit, skip); }
		α NeedsIdentityInsert()Ι->bool override{ return false; }
		α NowDefault()Ι->sv override{ return "(unixepoch())"; }
		α PrefixOut()Ι->bool override{ return false; }
		//No server-side procs - proc calls dispatch to native C++ registered in SqliteProcs.h, so Proc* generators are unused.
		α ProcParameterPrefix()Ι->sv override{ return {}; }
		α ProcStart()Ι->sv override{ return {}; }
		α ProcEnd()Ι->sv override{ return {}; }
		α SchemaDropsObjects()Ι->bool override{ return true; }
		α SchemaExistsSql()Ι->sv override{ return "select name from pragma_database_list where name=?"; } //'main' always exists - schema creation is a no-op.
		α SchemaSelect()Ι->sv override{ ASSERT_DESC(false, "sqlite does not have schemas"); return ""; }
		α SpecifyIndexCluster()Ι->bool override{ return false; }
		α SysSchema()Ι->sv override{ return "main"; }
		//rowid alias requires the declared type be exactly 'integer' - 'int'/'bigint' pks don't auto-assign. https://sqlite.org/lang_createtable.html#rowid
		α ToString( EType type )Ι->string override{ using enum EType; return type==Int || type==UInt || type==Long || type==ULong ? "integer" : Syntax::ToString( type ); }
		α UniqueIndexNames()Ι->bool override{ return true; } //index names are schema-wide in sqlite.
		α UtcNow()Ι->sv override{ return "unixepoch()"; }
	};
}
