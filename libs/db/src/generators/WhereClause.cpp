#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/Functions.h>

namespace Jde::DB{
	WhereClause::WhereClause( const Object& a, EOperator op, const Object& b, SL /*sl*/ )ε{
		auto clause = ToString( a );
		if( b.index()==underlying(EObject::Value) && get<Value>(b).is_null() ){
			if( op==EOperator::Equal )
				clause = Ƒ( "{} is null", move(clause) );
			else if( op==EOperator::NotEqual )
				clause = Ƒ( "{} is not null", move(clause) );
		}
		else{
			clause = Ƒ( "{} {} {}", move(clause), ToString(op), ToString(b) );
			auto aParams = GetParams( a );
			auto bParams = GetParams( b );
			move(aParams.begin(), aParams.end(), back_inserter(_params));
			move(bParams.begin(), bParams.end(), back_inserter(_params));
		}
		_clauses.push_back( move(clause) );
	}
	WhereClause::WhereClause( AliasCol&& c, Value::Underlying param, SL sl )ε:
		WhereClause{ move(c), EOperator::Equal, Value{move(param)}, sl }
	{}

	WhereClause::WhereClause( vector<WhereClause>&& clauses )ε{
		_clauses.reserve( clauses.size() );
		for( auto&& subTable : clauses ){
			move( begin(subTable._clauses), end(subTable._clauses), back_inserter(_clauses) );
			move( begin(subTable._params), end(subTable._params), back_inserter(_params) );
		}
	}

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

	α WhereClause::Add( const DB::Criteria& criteria )ε->void{
		Add( criteria.Column, EOperator::Equal, criteria.Value );
	}

	α WhereClause::Move()ι->string{
		string prefix{ _clauses.size() ? "where " : "" };
		return prefix + Str::Join( move(_clauses), " and " );
	}
	α WhereClause::Remove( sv clause )ι->void{
		for( auto it=_clauses.begin(); it!=_clauses.end(); ++it ){
			if( *it==clause ){
				_clauses.erase( it );
				break;
			}
		}
	}
	α WhereClause::Replace( sv tablePrefix1, sv tablePrefix2 )ι->void{
		for( auto&& clause : _clauses )
			clause = Str::Replace( clause, tablePrefix1, tablePrefix2 );
	}
}