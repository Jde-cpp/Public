#pragma once
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/FromClause.h> //complete Join for UsingClause.
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

namespace Jde::DB::MySql{
	//Driver-local dialect - resolved dynamically through IDataSource::Syntax(), so nothing in Jde.DB needs to know about it.
	//Header-only like Sqlite::SqliteSyntax: the tests include it directly, so no export from the MODULE is involved.
	struct MySqlSyntax final: Syntax{
		Ω Instance()->const MySqlSyntax&{ static const MySqlSyntax _instance; return _instance; }
		α AddDefault( sv tableName, sv columnName, Value dflt )Ι->string override{
			string v;
			if( dflt.is_bool() )
				v = dflt.get_bool() ? "true" : "false";
			else if( dflt.is_string() )
				v = dflt.get_string();
			else
				CRITICALT( ELogTags::Sql, "Default for index={} not implemented.", dflt.TypeName() );

			return Ƒ( "ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, v );
		}
		α AltDelimiter()Ι->sv override{ return "$$"; }
		α CanSetDefaultSchema()Ι->bool override{ return true; }
		α CatalogSelect()Ι->sv override{ return {}; }
		α CreatePrimaryKey( str tableName, str columns )Ι->string override{ return Ƒ("CONSTRAINT {}_pk PRIMARY KEY( {} )", tableName, columns); } //columns: comma-separated for composite keys.
		α DateTimeSelect( sv columnName )Ι->string override{ return Ƒ( "UNIX_TIMESTAMP({})", columnName ); }
		α DriverReturnsLastInsertId()Ι->bool override{ return true; }
		α EscapeDdl( sv sql )Ι->string override{
			const auto parts = Str::Split( sql, '.' );
			string y;
			for( const auto part : parts )
				y += '`'+string{part}+"`.";
			y.pop_back();
			return y;
		}
		α GuidType()Ι->sv override{ return "binary" ; }
		α HasLength( EType /*type*/ )Ι->bool override{ return true; }
		α HasCatalogs()Ι->bool override{ return false; }
		α HasUnsigned()Ι->bool override{ return true; }
		α IdentityColumnSyntax()Ι->sv override{ return "AUTO_INCREMENT"; }
		α IdentitySelect()Ι->sv override{ return "LAST_INSERT_ID()"; }
		α IsReservedWord( sv name )Ι->bool override{ return name=="groups"; } //mysql 8+ window-function keyword; the only reserved word used as an unprefixed table name.
		α Limit( str sql, uint limit, uint skip )Ι->string override{
			ASSERT( limit || skip );
			return skip
				? Ƒ("{} limit {} offset {}", sql, limit ? std::to_string(limit) : "18446744073709551615", skip)
				: Ƒ("{} limit {}", sql, limit); //MySQL: OFFSET requires a LIMIT; 2^64-1 = "all rows" when limit is unset (skip-only).
		}
		α NeedsIdentityInsert()Ι->bool override{ return false; }
		α NowDefault()Ι->sv override{ return "CURRENT_TIMESTAMP"; }
		α PrefixOut()Ι->bool override{ return true; }
		α ProcParameterPrefix()Ι->sv override{ return {}; }
		α ProcStart()Ι->sv override{ return "begin"; }
		α ProcEnd()Ι->sv override{ return "end"; }
		α SchemaDropsObjects()Ι->bool override{ return true; }
		α SchemaSelect()Ι->sv override{ return "select database() from dual;"; }
		α SpecifyIndexCluster()Ι->bool override{ return false; }
		α SysSchema()Ι->sv override{ return "sys"; }
		//`using(col)` only when both sides name the same unaliased column; anything else falls back to the base `on a.x=b.y`.
		α UsingClause( const Join& join )Ι->string override{
			const auto& c1 = *join.To;
			return join.From->Name==c1.Name && join.ToAlias.empty() && join.FromAlias.empty()
				? Ƒ( "\n\t{}join {} using({})", join.Inner ? "" : "left ", c1.Table->SqlName(), c1.Name )
				: Syntax::UsingClause( join );
		}
		α UtcNow()Ι->sv override{ return "CURRENT_TIMESTAMP()"; }
		α ZeroSequenceMode()Ι->sv override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"; }
	};
}
