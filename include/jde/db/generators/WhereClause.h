#pragma once
#ifndef WHERE_CLAUSE_H
#define WHERE_CLAUSE_H
#include <jde/db/Value.h>
#include <jde/db/generators/Syntax.h>

namespace Jde::DB{
	struct Column;
	struct WhereClause final{
		WhereClause()ι=default;
		//WhereClause( string init )ι{ Add(move(init)); }
		WhereClause( sp<Column> columnName, Value param, SRCE )ε{ Add(columnName, move(param), sl); }
		WhereClause( sp<Column> columnName, vector<Value> inParams, SRCE )ε{ Add(columnName, move(inParams), sl); }
		//friend α operator<<( WhereClause &self, string clause )ι->WhereClause&{ self.Add(move(clause)); return self; }
		α operator+=( const WhereClause& subTable )ι->WhereClause&;

		α Add( sp<Column> col, EOperator op, Value param, SRCE )ε->void;
		α Add( sp<Column> col, EOperator op, vector<Value> inParams, SRCE )ε->void;

		α Add( sp<Column> col, Value param, SRCE )ε->void{ Add( col, EOperator::Equal, move(param), sl ); }
		α Add( sp<Column> col, vector<Value> inParams, SRCE )ε->void{ Add( col, EOperator::In, move(inParams), sl ); }
		α Add( string clause )ε{ _clauses.push_back(move(clause)); }
		α Empty()Ι->bool{ return _clauses.empty(); }
		α Move()ι->string;

		α Params()ι->vector<Value>&{ return _params; }
		α Params()Ι->const vector<Value>&{ return _params; }
		α Size()Ι->uint{ return _clauses.size(); }
	private:
		vector<string> _clauses;
		vector<Value> _params;
	};
}
#endif