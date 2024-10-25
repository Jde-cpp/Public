#pragma once
#include <jde/db/generators/WhereClause.h>

namespace Jde::DB{ struct Value; struct View; }

namespace Jde::QL{
	struct TableQL;
	α ToString( DB::EOperator op )ι->string;
	α ToQLOperator( string op )ι->DB::EOperator;

	struct FilterValueQL final{
		DB::EOperator Operator;
		jvalue Value;
		α Test( const DB::Value& value, ELogTags logTags )Ι->bool;
	};
	using JColName=string;
	struct FilterQL final{//filter:
		flat_map<JColName,vector<FilterValueQL>> ColumnFilters;
		Ω Test( const DB::Value& value, const vector<FilterValueQL>& filters, ELogTags logTags )ι->bool;
	};
	α ToWhereClause( const TableQL& table, const DB::View& schemaTable )->DB::WhereClause;
}