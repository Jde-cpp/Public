#pragma once
#include <jde/db/generators/WhereClause.h>

namespace Jde::DB{ struct Value; struct View; }

namespace Jde::QL{
	struct TableQL;
	α ToString( DB::EOperator op )ι->string;
	α ToQLOperator( string op )ι->DB::EOperator;

	struct FilterValue final{
		DB::EOperator Operator;
		jvalue Value;
		α Test( const DB::Value& value, ELogTags logTags )Ι->bool;
		Ŧ Test( T value )Ι->bool;
	};
	using JColName=string;
	struct Filter final{
		α Empty()Ι->bool{ return ColumnFilters.empty() /*&& !StartTime && !EndTime*/; }
		Ŧ Test( str columnName, const T& value )Ι->bool;
		Ŧ TestF( str columnName, function<T()> f )Ι->bool;
		Ω Test( const DB::Value::Underlying& value, const vector<FilterValue>& filters, ELogTags logTags )ι->bool;
		flat_map<JColName,vector<FilterValue>> ColumnFilters;
	};
	α ToWhereClause( const TableQL& table, const DB::View& schemaTable, bool includeDeleted=false )->DB::WhereClause;

	template<> Ξ FilterValue::Test( string value )Ι->bool{
		return Test( DB::Value{move(value)}, ELogTags::QL );
	}
	Ŧ FilterValue::Test( T value )Ι->bool{
		return Test( DB::Value{move(value)}, ELogTags::QL );
	}
	Ŧ Filter::Test( str columnName, const T& value )Ι->bool{
		if( auto it = ColumnFilters.find(columnName); it!=ColumnFilters.end() ){
			for( const auto& filterValue : it->second ){
				if( !filterValue.Test(value) )
					return false;
			}
		}
		return true;
	}
	Ŧ Filter::TestF( str columnName, function<T()> f )Ι->bool{
		if( auto it = ColumnFilters.find(columnName); it!=ColumnFilters.end() ){
			auto value = f();
			for( const auto& filterValue : it->second ){
				if( !filterValue.Test(value) )
					return false;
			}
		}
		return true;
	}
}