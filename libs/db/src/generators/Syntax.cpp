#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/FromClause.h>
#define let const auto

namespace Jde{
	inline constexpr std::array<sv,11> OperatorStrings = { "=", "!=", "regex", "glob", "in", "nin", ">", ">=", "<", "<=", "elemMatch" };
	α DB::ToOperator( sv op )ι->EOperator{ return ToEnum<EOperator>( OperatorStrings, op ).value_or( EOperator::Equal ); }
	α DB::ToString( EOperator op )ι->string{ return FromEnum<EOperator>( OperatorStrings, op ); }
}
namespace Jde::DB{
	constexpr ELogTags _tags{ ELogTags::Sql };
	static Syntax _sqlInstance;
	static MySqlSyntax _mySqlInstance;
	α Syntax::Instance()->const Syntax&{ return _sqlInstance; }
	α MySqlSyntax::Instance()->const MySqlSyntax&{ return _mySqlInstance; }

	α Syntax::FormatOperator( const Column& col, EOperator op, uint size, SL )Ε->string{
		if( op!=EOperator::In && op!=EOperator::NotIn )
			return Ƒ( "{}{}?", col.FQName(), OperatorStrings[(uint)op] );

		if( size==0 ) //`col in ()` is invalid SQL; an empty IN matches nothing, an empty NOT IN matches everything.
			return op==EOperator::NotIn ? "1=1" : "1=0";

		string params;
		for( uint i=0; i<size; ++i )
			params += "?,";
		params.pop_back();
		return Ƒ( "{} {}({})", col.FQName(), op==EOperator::NotIn ? "not in" : "in", params );
	}

	α Syntax::AddDefault( sv tableName, sv columnName, Value dflt )Ι->string{
		string v;
		if( dflt.is_bool() )
			v = dflt.get_bool() ? "1" : "0";
		else if( dflt.is_string() )
			v = dflt.get_string();
		else
			CRITICALT( ELogTags::Sql, "Default for '{}' not implemented.", dflt.TypeName() );

		return Ƒ("alter table {} add default {} for {}", tableName, v, columnName);
	}

	α MySqlSyntax::AddDefault( sv tableName, sv columnName, Value dflt )Ι->string{
		string v;
		if( dflt.is_bool() )
			v = dflt.get_bool() ? "true" : "false";
		else if( dflt.is_string() )
			v = dflt.get_string();
		else
			CRITICALT( ELogTags::Sql, "Default for index={} not implemented.", dflt.TypeName() );

		return Ƒ( "ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, v );
	}

	α Syntax::EscapeDdl( sv sql )Ι->string{
		let parts = Str::Split( sql, '.' );
		string y;
		for( let part : parts )
			y += '['+string{part}+"].";
		y.pop_back();
		return y;
	}
	α MySqlSyntax::EscapeDdl( sv sql )Ι->string{
		let parts = Str::Split( sql, '.' );
		string y;
		for( let part : parts )
			y += '`'+string{part}+"`.";
		y.pop_back();
		return y;
	}
	α Syntax::HasLength( EType type )Ι->bool{
		using enum EType;
		return type == VarChar || type == Binary || type == Char || type == VarBinary;
	}

	α Syntax::Limit( str input, uint limit, uint skip )Ε->string{
		ASSERT( limit || skip );
		string sql = input+" offset "+std::to_string(skip)+" rows"; //T-SQL: OFFSET is mandatory before FETCH (emit it even for skip==0); both require an ORDER BY on the statement.
		if( limit )
			sql += " fetch next "+std::to_string(limit)+" rows only";
		return sql;
	}
	α MySqlSyntax::Limit( str sql, uint limit, uint skip )Ι->string{
		ASSERT( limit || skip );
		return skip
			? Ƒ("{} limit {} offset {}", sql, limit ? std::to_string(limit) : "18446744073709551615", skip)
			: Ƒ("{} limit {}", sql, limit); //MySQL: OFFSET requires a LIMIT; 2^64-1 = "all rows" when limit is unset (skip-only).
	}
	α joinType( bool inner )ι->string{
		return inner ? "" : "left ";
	}
	α Syntax::UsingClause( const Join& join )Ι->string{
		let& c0 = *join.From;
		let& c1 = *join.To;
		return Ƒ( "\n\t{0}join {3}{4} on {1}.{2}={5}.{6}",
			joinType(join.Inner),
			join.FromAlias.empty() ? c0.Table->DBName : join.FromAlias,
			c0.Name,
			c1.Table->DBName,
			join.ToAlias.empty() ? "" : " "+join.ToAlias,
			join.ToAlias.empty() ? c1.Table->DBName : join.ToAlias,
			c1.Name );
	}
	α MySqlSyntax::UsingClause( const Join& join )Ι->string{
		let& c1 = *join.To;
		return join.From->Name==c1.Name && join.ToAlias.empty() && join.FromAlias.empty()
			? Ƒ( "\n\t{}join {} using({})", joinType(join.Inner), c1.Table->DBName, c1.Name )
			: Syntax::UsingClause( join );
	}

	α Syntax::ToString( EType type )Ι->string{
		using enum EType;
		string typeName;
		if( HasUnsigned() && type == UInt ) typeName = "int unsigned";
		else if( type == Int || type == UInt ) typeName = "int";
		else if( HasUnsigned() && type == ULong ) typeName = "bigint(20) unsigned";
		else if( type == Long || type == ULong ) typeName="bigint";
		else if( type == DateTime ) typeName = "datetime";
		else if( type == SmallDateTime )typeName = "smalldatetime";
		else if( type == Float ) typeName = "float";
		else if( type == SmallFloat )typeName = "real";
		else if( type == VarWChar ) typeName = "nvarchar";
		else if( type == WChar ) typeName = "nchar";
		else if( HasUnsigned() && type == UInt16 ) typeName="smallint unsigned";
		else if( type == Int16 || type == UInt16 ) typeName="smallint";
		else if( HasUnsigned() && type == UInt8 ) typeName =  "tinyint unsigned";
		else if( type == Int8 || type == UInt8 ) typeName = "tinyint";
		else if( type == Guid ) typeName = GuidType();
		else if( type == VarBinary ) typeName = "varbinary";
		else if( type == VarChar ) typeName = "varchar";
		else if( type == NText ) typeName = "ntext";
		else if( type == Text ) typeName = "text";
		else if( type == Char ) typeName = "char";
		else if( type == Image ) typeName = "image";
		else if( type == Bit ) typeName="bit";
		else if( type == Binary ) typeName = "binary";
		else if( type == Decimal ) typeName = "decimal";
		else if( type == Numeric ) typeName = "numeric";
		else if( type == Money ) typeName = "money";
		else ERR( "Unknown datatype({}).", (uint)type );
		return typeName;
	}
}