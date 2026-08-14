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
	α Syntax::Instance()->const Syntax&{ return _sqlInstance; }

	//OperatorStrings does double duty - it is the wire/display spelling for ToOperator/ToString *and* was indexed here for
	//SQL.  Only its six punctuation entries are valid SQL: `regex`/`glob`/`elemMatch` are names, and the old
	//`Ƒ("{}{}?", FQName, OperatorStrings[op])` glued them straight onto the column ("users.nameregex?"), which lexes as
	//one identifier.  The comparison operators are listed explicitly now, so a word operator added to the enum later
	//lands in the default and throws instead of silently emitting a broken clause.
	α Syntax::FormatOperator( const Column& col, EOperator op, uint size, SL sl )Ε->string{
		using enum EOperator;
		switch( op ){
		case Equal: case NotEqual: case Greater: case GreaterOrEqual: case Less: case LessOrEqual:
			return Ƒ( "{}{}?", col.FQName(), OperatorStrings[(uint)op] ); //punctuation: self-delimiting, no spaces needed.
		case In: case NotIn:
			if( size==0 ) //`col in ()` is invalid SQL; an empty IN matches nothing, an empty NOT IN matches everything.
				return op==NotIn ? "1=1" : "1=0";
			else{
				string params;
				for( uint i=0; i<size; ++i )
					params += "?,";
				params.pop_back();
				return Ƒ( "{} {}({})", col.FQName(), op==NotIn ? "not in" : "in", params );
			}
		case Regex: case Glob:
			//A word operator needs its spaces.  One pattern, one placeholder: an array literal (`glob:["a*","b*"]`)
			//would otherwise render one '?' against N bound params and silently misalign the whole statement.
			if( size!=1 )
				throw Exception{ sl, Jde::ELogLevel::Debug, "'{}' takes a single pattern, not {}.", DB::ToString(op), size };
			return Ƒ( "{} {} ?", col.FQName(), PatternOperator(op, sl) );
		default:
			throw Exception{ sl, Jde::ELogLevel::Debug, "Operator '{}' has no SQL form.", DB::ToString(op) };
		}
	}

	α Syntax::PatternOperator( EOperator op, SL sl )Ε->sv{
		if( op==EOperator::Glob )
			return "like"; //T-SQL LIKE is glob's language with '%'/'_' for '*'/'?' - PatternParam does that rewrite.
		throw Exception{ sl, Jde::ELogLevel::Debug, "SQL Server has no '{}' operator (REGEXP_LIKE is 2025+); use 'glob'.", DB::ToString(op) };
	}
	α Syntax::PatternParam( EOperator op, str pattern, SL sl )Ε->string{
		if( op==EOperator::Glob )
			return GlobToLike( pattern );
		throw Exception{ sl, Jde::ELogLevel::Debug, "SQL Server has no '{}' operator (REGEXP_LIKE is 2025+); use 'glob'.", DB::ToString(op) };
	}

	//'*'->'%' and '?'->'_'; a literal '%'/'_' becomes a one-element class ('[%]'), which is how T-SQL escapes them
	//without an ESCAPE clause - and which QL::globMatch already reads the same way, so the two stay in step.  Classes
	//pass through: T-SQL LIKE has them, spelled '[^…]' where glob also accepts '[!…]'.
	α GlobToLike( sv glob )ι->string{
		string y;
		for( uint i{}; i<glob.size(); ++i ){
			let c = glob[i];
			if( c=='*' )
				y += '%';
			else if( c=='?' )
				y += '_';
			else if( c=='%' || c=='_' )
				y += Ƒ( "[{}]", c );
			else if( c=='[' ){
				let close = glob.find( ']', i+2+(i+1<glob.size() && (glob[i+1]=='^'||glob[i+1]=='!') ? 1 : 0) );
				if( close==sv::npos ) //unterminated: a literal '[', as in sqlite - and '[' is a LIKE metacharacter too.
					y += "[[]";
				else{
					y += '[';
					if( glob[i+1]=='!' || glob[i+1]=='^' ){ //glob accepts both spellings of negation; T-SQL only '^'.
						y += '^';
						++i;
					}
					y.append( glob.substr(i+1, close-i-1) );
					y += ']';
					i = close;
				}
			}
			else
				y += c;
		}
		return y;
	}

	//'*'->'.*', '?'->'.', classes pass through, everything else escaped, and the whole thing anchored - glob matches the
	//entire value, where REGEXP is a search.
	α GlobToRegex( sv glob )ι->string{
		constexpr sv metacharacters{ ".^$*+?()[]{}|\\" };
		string y{ "^" };
		for( uint i{}; i<glob.size(); ++i ){
			let c = glob[i];
			if( c=='*' )
				y += ".*";
			else if( c=='?' )
				y += '.';
			else if( c=='[' ){
				let close = glob.find( ']', i+2+(i+1<glob.size() && (glob[i+1]=='^'||glob[i+1]=='!') ? 1 : 0) );
				if( close==sv::npos ) //unterminated: a literal '[', as in sqlite.
					y += "\\[";
				else{
					y += '[';
					if( glob[i+1]=='!' || glob[i+1]=='^' ){
						y += '^';
						++i;
					}
					y.append( glob.substr(i+1, close-i-1) ); //ranges and members are spelled the same in both languages.
					y += ']';
					i = close;
				}
			}
			else{
				if( metacharacters.find(c)!=sv::npos )
					y += '\\';
				y += c;
			}
		}
		return y+"$";
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

	α Syntax::EscapeDdl( sv sql )Ι->string{
		let parts = Str::Split( sql, '.' );
		string y;
		for( let part : parts )
			y += '['+string{part}+"].";
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
	α joinType( bool inner )ι->string{
		return inner ? "" : "left ";
	}
	α Syntax::UsingClause( const Join& join )Ι->string{
		let& c0 = *join.From;
		let& c1 = *join.To;
		return Ƒ( "\n\t{0}join {3}{4} on {1}.{2}={5}.{6}",
			joinType(join.Inner),
			join.FromAlias.empty() ? c0.Table->SqlName() : join.FromAlias,
			c0.Name,
			c1.Table->SqlName(),
			join.ToAlias.empty() ? "" : " "+join.ToAlias,
			join.ToAlias.empty() ? c1.Table->SqlName() : join.ToAlias,
			c1.Name );
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