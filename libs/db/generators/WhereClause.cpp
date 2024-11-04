#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

namespace Jde::DB{
	α WhereClause::operator+=( const WhereClause& subTable )ι->WhereClause&{
		_clauses.insert( end(_clauses), begin(subTable._clauses), end(subTable._clauses) );
		_params.insert( end(_params), begin(subTable._params), end(subTable._params) );
		return *this;
	}
	α WhereClause::Add( sp<Column> col, EOperator op, Value param, SL sl )ε->void{
		if( param.is_null() ){
			string prefix;
			if( op==EOperator::NotEqual )
				prefix = "not ";
			else if( op!=EOperator::Equal )
				throw Exception{ sl, Jde::ELogLevel::Debug, "Null value not allowed for operator '{}'.", ToString(op) };
			_clauses.push_back( Ƒ("{} is {}null", col->FQName(), prefix) );
		}else{
			_clauses.push_back( col->Table->Syntax().FormatOperator(*col, op, 1, sl) );
			_params.push_back(move(param));
		}
	}

	α WhereClause::Add( sp<Column> col, EOperator op, vector<Value> inParams, SL sl )ε->void{
		for( auto& param : inParams )
			_params.emplace_back( move(param) );
		_clauses.push_back( col->Table->Syntax().FormatOperator(*col, op, inParams.size(), sl) );
	}

	α WhereClause::Move()ι->string{
		string prefix{ _clauses.size() ? "where " : "" };
		return prefix + Str::Join( move(_clauses), " and " );
	}
}