#pragma once
#ifndef SYNTAX_H
#define SYNTAX_H
#include <jde/fwk/str.h>
#include <jde/db/Value.h>
#include "../exports.h"

#define Φ ΓDB α
namespace Jde::DB{
	struct Column; struct Join; struct Table;
	enum class EOperator : uint8{Equal,NotEqual,Regex,Glob,In,NotIn,Greater,GreaterOrEqual,Less,LessOrEqual,ElementMatch};
	Φ ToOperator( sv op )ι->EOperator;
	Φ ToString( EOperator op )ι->string;

	//`glob:` is the one pattern language the QL filters speak - sqlite GLOB semantics, matched in-memory by QL::globMatch.
	//Only sqlite takes it verbatim, so the other dialects translate the *pattern* as well as swapping the keyword.
	//Case sensitivity does not survive the trip and cannot: GLOB is case-sensitive, T-SQL LIKE and MySQL REGEXP both
	//follow the column collation (usually case-insensitive).  A pattern may therefore match more rows in SQL than the
	//same pattern matches in the in-memory filter.
	Φ GlobToLike( sv glob )ι->string;  //T-SQL LIKE: '*'->'%', '?'->'_', a literal '%'/'_' bracket-escaped, '[!…]'->'[^…]'.
	Φ GlobToRegex( sv glob )ι->string; //anchored regex, for dialects with REGEXP but no GLOB.

	struct ΓDB Syntax{
		Ω Instance()->const Syntax&;
		virtual ~Syntax()=default;
		α FormatOperator( const Column& col, EOperator op, uint size=1, SRCE )Ε->string;
		//Regex/Glob bind their pattern as a parameter, so a dialect has to state both halves - the keyword, and how the
		//pattern must be spelled for that keyword.  Throwing is a legitimate answer: no dialect implements all of them.
		β PatternOperator( EOperator op, SRCE )Ε->sv;              //SQL Server: LIKE, but no regex before 2025.
		β PatternParam( EOperator op, str pattern, SRCE )Ε->string;
		β AddDefault( sv tableName, sv columnName, Value dflt )Ι->string;
		β AltDelimiter()Ι->sv{ return {}; }
		β CanAddForeignKeys()Ι->bool{ return true; } //false (sqlite): no 'alter table add constraint' - fks only enforced when inline in create table.
		β CanSetDefaultSchema()Ι->bool{ return false; }
		β CatalogSelect()Ι->sv{ return "select db_name();"; }
		β CreatePrimaryKey( str tableName, str columns )Ι->string{ return Ƒ("CONSTRAINT {}_pk PRIMARY KEY( {} )", tableName, columns); } //columns: comma-separated for composite keys.
		β DateTimeSelect( sv columnName )Ι->string{ return string{ columnName }; }
		β DriverReturnsLastInsertId()Ι->bool{ return true; }
		β EscapeDdl( sv sql )Ι->string;
		β GuidType()Ι->sv{ return "uniqueidentifier"; }
		β HasLength( EType type )Ι->bool;
		β HasCatalogs()Ι->bool{ return true; }
		β HasProcs()Ι->bool{ return true; }
		β HasSchemas()Ι->bool{ return true; }
		β HasUnsigned()Ι->bool{ return false; }
		β IdentityColumnSyntax()Ι->sv{ return "identity(1001,1)"; }
		β IdentitySelect()Ι->sv{ return "@@identity"; }
		β IndexName( sv /*tableName*/, sv indexName )Ι->string{ return string{indexName}; } //per-table index namespace; schema-wide dialects qualify with the table.
		β IsReservedWord( sv /*name*/ )Ι->bool{ return false; } //only words actually used as unprefixed object names - extend the dialect override when a new collision appears.
		β Limit( str syntax, uint limit, uint skip )Ε->string;
		β NeedsIdentityInsert()Ι->bool{ return true; }
		β NowDefault()Ι->sv{ return UtcNow(); }
		β PrefixOut()Ι->bool{ return false; }
		β ProcParameterPrefix()Ι->sv{ return "@"; }
		β ProcStart()Ι->sv{ return "as\n\tset nocount on;\n"; }
		β ProcEnd()Ι->sv{ return {}; }
		β SchemaDropsObjects()Ι->bool{ return false; }
		β SchemaExistsSql()Ι->sv{ return "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?"; }
		β QualifiedName( sv schema, sv name )Ι->string{ return HasSchemas() ? Ƒ("{}.{}", schema, name) : string{name}; } //fully-qualified object name; schemaless dialects use the bare name.
		β SchemaSelect()Ι->sv{ return "select schema_name();"; } //empty (like CatalogSelect): SchemaName falls back to SysSchema without querying.
		β SpecifyIndexCluster()Ι->bool{ return true; }
		β SysSchema()Ι->sv{ return "dbo"; }
		β ToString( EType type )Ι->string;

		β UniqueIndexNames()Ι->bool{ return false; }
		β UsingClause( const Join& join )Ι->string;
		β UtcNow()Ι->sv{ return "getutcdate()"; }
		β ZeroSequenceMode()Ι->sv{ return {}; }
	};
	//The MySQL dialect lives with its driver, in libs/db/drivers/mysql/src/MySqlSyntax.h - like Sqlite::SqliteSyntax,
	//it is only reached through IDataSource::Syntax(), so Jde.DB never needs the type statically.
}
#undef Φ
#endif