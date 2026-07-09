#pragma once
#include <jde/db/generators/Syntax.h>

namespace Jde::DB::Sqlite{
	//Driver-local dialect - resolved dynamically through IDataSource::Syntax(), so nothing in Jde.DB needs to know about it.
	//Could move next to MySqlSyntax in include/jde/db/generators/Syntax.h if DDL/generators ever need it statically.
	struct SqliteSyntax final : Syntax{
		Ω Instance()->const SqliteSyntax&{ static const SqliteSyntax _instance; return _instance; }
		α AltDelimiter()Ι->sv override{ return {}; }
		α CanSetDefaultSchema()Ι->bool override{ return false; } //single db per connection; ATTACH could emulate schemas.
		α CatalogSelect()Ι->sv override{ return {}; }
		α DateTimeSelect( sv columnName )Ι->string override{ return string{columnName}; } //stored as unix epoch integer - see SqliteRow.
		α DriverReturnsLastInsertId()Ι->bool override{ return true; } //sqlite3_last_insert_rowid.
		α EscapeDdl( sv sql )Ι->string override{ return Ƒ( "\"{}\"", sql ); } //TODO: multi-part names like base impl.
		α GuidType()Ι->sv override{ return "blob"; }
		α HasLength( EType )Ι->bool override{ return false; } //type affinity - lengths are documentation only.
		α HasCatalogs()Ι->bool override{ return false; }
		α HasUnsigned()Ι->bool override{ return false; }
		α IdentityColumnSyntax()Ι->sv override{ return {}; } //rowid alias: pk must be declared 'integer primary key' - see CreatePrimaryKey.
		α IdentitySelect()Ι->sv override{ return "last_insert_rowid()"; }
		α CreatePrimaryKey( str /*tableName*/, str columnName )Ι->string override{ return Ƒ("PRIMARY KEY( {} )", columnName); }
		α Limit( str sql, uint limit, uint skip )Ι->string override{ return Ƒ("{} limit {} offset {}", sql, limit, skip); }
		α NeedsIdentityInsert()Ι->bool override{ return false; }
		α NowDefault()Ι->sv override{ return "(unixepoch())"; }
		α PrefixOut()Ι->bool override{ return false; }
		//No server-side procs - proc calls dispatch to native C++ registered in SqliteProcs.h, so Proc* generators are unused.
		α ProcParameterPrefix()Ι->sv override{ return {}; }
		α ProcStart()Ι->sv override{ return {}; }
		α ProcEnd()Ι->sv override{ return {}; }
		α SchemaDropsObjects()Ι->bool override{ return true; }
		α SchemaSelect()Ι->sv override{ return "select 'main';"; }
		α SpecifyIndexCluster()Ι->bool override{ return false; }
		α SysSchema()Ι->sv override{ return "main"; }
		α UniqueIndexNames()Ι->bool override{ return true; } //index names are schema-wide in sqlite.
		α UtcNow()Ι->sv override{ return "unixepoch()"; }
	};
}
