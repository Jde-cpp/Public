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
	struct Syntax{
		ΓDB Ω Instance()->const Syntax&;
		virtual ~Syntax()=default;
		α FormatOperator( const Column& col, EOperator op, uint size=1, SRCE )Ε->string;
		β AddDefault( sv tableName, sv columnName, Value dflt )Ι->string;
		β AltDelimiter()Ι->sv{ return {}; }
		β CanSetDefaultSchema()Ι->bool{ return false; }
		β CatalogSelect()Ι->sv{ return "select db_name();"; }
		β CreatePrimaryKey( str tableName, str columnName )Ι->string{ return Ƒ("CONSTRAINT {}_pk PRIMARY KEY( {} )", tableName, columnName); }
		β DateTimeSelect( sv columnName )Ι->string{ return string{ columnName }; }
		β DriverReturnsLastInsertId()Ι->bool{ return true; }
		β EscapeDdl( sv sql )Ι->string;
		β GuidType()Ι->sv{ return "uniqueidentifier"; }
		β HasLength( EType type )Ι->bool;
		β HasCatalogs()Ι->bool{ return true; }
		β HasUnsigned()Ι->bool{ return false; }
		β IdentityColumnSyntax()Ι->sv{ return "identity(1001,1)"; }
		β IdentitySelect()Ι->sv{ return "@@identity"; }
		β Limit( str syntax, uint limit )Ε->string;
		β NeedsIdentityInsert()Ι->bool{ return true; }
		β NowDefault()Ι->sv{ return UtcNow(); }
		β PrefixOut()Ι->bool{ return false; }
		β ProcParameterPrefix()Ι->sv{ return "@"; }
		β ProcStart()Ι->sv{ return "as\n\tset nocount on;\n"; }
		β ProcEnd()Ι->sv{ return {}; }
		β SchemaDropsObjects()Ι->bool{ return false; }
		β SchemaSelect()Ι->sv{ return "select schema_name();"; }
		β SpecifyIndexCluster()Ι->bool{ return true; }
		β SysSchema()Ι->sv{ return "dbo"; }
		α ToString( EType type )Ι->string;

		β UniqueIndexNames()Ι->bool{ return false; }
		β UsingClause( const Join& join )Ι->string;
		β UtcNow()Ι->sv{ return "getutcdate()"; }
		β ZeroSequenceMode()Ι->sv{ return {}; }
	};

	struct MySqlSyntax final: Syntax{
		Ω Instance()->const MySqlSyntax&;
		α AddDefault( sv tableName, sv columnName, Value dflt )Ι->string;
		α AltDelimiter()Ι->sv override{ return "$$"; }
		α CanSetDefaultSchema()Ι->bool{ return true; }
		α CatalogSelect()Ι->sv override{ return {}; }
		α CreatePrimaryKey( str tableName, str columnName )Ι->string{ return Ƒ("CONSTRAINT {}_pk PRIMARY KEY( {} )", tableName, columnName); }
		α DateTimeSelect( sv columnName )Ι->string override{ return Ƒ( "UNIX_TIMESTAMP({})", columnName ); }
		α DriverReturnsLastInsertId()Ι->bool override{ return true; }
		α EscapeDdl( sv sql )Ι->string;
		α GuidType()Ι->sv{ return "binary"; }
		α HasLength( EType /*type*/ )Ι->bool { return true; }
		α HasCatalogs()Ι->bool{ return false; }
		α HasUnsigned()Ι->bool override{ return true; }
		α IdentityColumnSyntax()Ι->sv override{ return "AUTO_INCREMENT"; }
		α IdentitySelect()Ι->sv override{ return "LAST_INSERT_ID()"; }
		α Limit( str sql, uint limit )Ι->string override{ return Ƒ("{} limit {}", sql, limit); }
		α NeedsIdentityInsert()Ι->bool override{ return false; }
		α NowDefault()Ι->sv override{ return "CURRENT_TIMESTAMP"; }
		α PrefixOut()Ι->bool{ return true; }
		α ProcParameterPrefix()Ι->sv override{ return {}; }
		α ProcStart()Ι->sv override{ return "begin"; }
		α ProcEnd()Ι->sv override{ return "end"; }
		α SchemaDropsObjects()Ι->bool override{ return true; }
		α SchemaSelect()Ι->sv override{ return "select database() from dual;"; }
		α SpecifyIndexCluster()Ι->bool override{ return false; }
		α SysSchema()Ι->sv override{ return "sys"; }
		α UsingClause( const Join& join )Ι->string override;
		α UtcNow()Ι->sv override{ return "CURRENT_TIMESTAMP()"; }
		α ZeroSequenceMode()Ι->sv override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"; }
	};
}
#undef Φ
#endif