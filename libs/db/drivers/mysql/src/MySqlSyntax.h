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
		α CanSetDefaultSchema()Ι->bool override{ return true; }
		α CatalogSelect()Ι->sv override{ return {}; }
		α CreatePrimaryKey( str tableName, str columns )Ι->string override{ return Ƒ("CONSTRAINT {}_pk PRIMARY KEY( {} )", tableName, columns); } //columns: comma-separated for composite keys.
		α DateTimeSelect( sv columnName )Ι->string override{ return Ƒ( "UNIX_TIMESTAMP({})", columnName ); }
		α QuoteChars()Ι->std::pair<char,char> override{ return {'`','`'}; }
		α GuidType()Ι->sv override{ return "binary" ; }
		α ToString( EType type )Ι->string override{
			using enum EType;
			return type==Float ? "double" : type==SmallFloat ? "float" : type==Blob ? "blob" : Syntax::ToString( type );
		}
		α HasLength( EType type )Ι->bool override{
			using enum EType;
			return Syntax::HasLength( type ) || type==Bit || type==Guid;
		}
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
		//The only dialect with a real regex engine (ICU, since 8.0), so `regex:` passes through.  There is no GLOB, and
		//LIKE would lose the character classes, so `glob:` is translated to an anchored REGEXP rather than to LIKE - that
		//mapping is total, where glob->LIKE is not.  Both are collation-cased, unlike the case-sensitive in-memory match.
		α PatternOperator( EOperator op, SRCE )Ε->sv override{
			if( op==EOperator::Regex || op==EOperator::Glob )
				return "regexp";
			throw Exception{ sl, Jde::ELogLevel::Debug, "Operator '{}' has no SQL form.", DB::ToString(op) };
		}
		α PatternParam( EOperator op, str pattern, SRCE )Ε->string override{
			PatternOperator( op, sl );
			return op==EOperator::Regex
				? pattern //ICU dialect: near enough to ECMAScript for the patterns the filters accept.
				: GlobToRegex( pattern );
		}
		α PrefixOut()Ι->bool override{ return true; }
		//`values(col)` rather than the 8.0.19 `as new` alias: MariaDB, which this driver's error categories also cover,
		//only understands the older form.
		α UpsertSuffix( const vector<sv>& /*keyColumns*/, const vector<sv>& updateColumns )Ι->string override{
			string y{ " on duplicate key update " };
			for( uint i=0; i<updateColumns.size(); ++i )
				y += Ƒ( "{}{}=values({})", i ? ", " : "", updateColumns[i], updateColumns[i] );
			return y;
		}
		α CreateProcSql()Ι->sv override{ return "create procedure"; } //MySQL has no create-or-replace for procedures.
		α DropProcSql( sv qualifiedName )Ι->string override{ return Ƒ("drop procedure if exists {}", qualifiedName); }
		α ProcParameterPrefix()Ι->sv override{ return {}; }
		α ProcStart()Ι->sv override{ return "begin"; }
		α ProcEnd()Ι->sv override{ return "end"; }
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
	};
}
