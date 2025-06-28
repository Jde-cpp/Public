#pragma once
#ifndef WHERE_CLAUSE_H
#define WHERE_CLAUSE_H
#include "../exports.h"
#include "../Value.h"
#include "Syntax.h"
#include "AliasColumn.h"
#include "Object.h"

namespace Jde::DB{
	struct Column;
	struct Criteria;
	struct ΓDB WhereClause final{
		WhereClause()ι=default;
		WhereClause( sp<Column> c, Value::Underlying param, SRCE )ε{ Add(c, move(param), sl); }
		WhereClause( AliasCol&& c, Value::Underlying param, SRCE )ε;
		WhereClause( sp<Column> c, vector<Value> inParams, SRCE )ε{ Add(c, move(inParams), sl); }
		WhereClause( const Object& a, EOperator op, const Object& b, SRCE )ε;
		WhereClause( vector<WhereClause>&& clauses )ε;
		α operator+=( const WhereClause& subTable )ι->WhereClause&;

		α Add( sp<Column> col, EOperator op, Value param, SRCE )ε->void;
		α Add( sp<Column> col, EOperator op, Value::Underlying param, SRCE )ε->void{ Add( col, op, Value{move(param)}, sl ); }
		α Add( sp<Column> col, EOperator op, vector<Value> inParams, SRCE )ε->void;

		α Add( sp<Column> col, Value::Underlying param, SRCE )ε->void{ Add( col, EOperator::Equal, move(param), sl ); }
		α Add( sp<Column> col, Value param, SRCE )ε->void{ Add( col, EOperator::Equal, move(param.Variant), sl ); }
		α Add( sp<Column> col, vector<Value> inParams, SRCE )ε->void{ Add( col, EOperator::In, move(inParams), sl ); }
		α Add( string clause )ε{ _clauses.push_back(move(clause)); }
		α Add( const DB::Criteria& criteria )ε->void;
		α Empty()Ι->bool{ return _clauses.empty(); }
		α Move()ι->string;

		α Params()ι->vector<Value>&{ return _params; }
		α Params()Ι->const vector<Value>&{ return _params; }
		α Remove( sv clause )ι->void;
		α Replace( sv tablePrefix1, sv tablePrefix2 )ι->void;
		α Size()Ι->uint{ return _clauses.size(); }
	private:
		vector<string> _clauses;
		vector<Value> _params;
	};
}
#endif