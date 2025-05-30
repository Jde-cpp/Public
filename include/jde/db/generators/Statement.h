#pragma once
#include "../exports.h"
#include "FromClause.h"
#include "SelectClause.h"
#include "Sql.h"
#include "WhereClause.h"

namespace Jde::DB{
	struct Value;
	struct ΓDB Statement final{
//		Statement( string&& sql, vector<DB::Value>&& params )ι:Sql{move(sql)}, Parameters{params} {}
		Statement()ι=default;
		Statement( SelectClause&& select, FromClause&& from, WhereClause&& where )ι:Select{move(select)}, From{move(from)}, Where{move(where)} {}
		α Empty()ι->bool{ return Select.Columns.empty(); }
		α Limit( uint limit )ι->void{ _limit=limit; }
		α Move()ε->Sql;
		SelectClause Select;
		FromClause From;
		WhereClause Where;
	private:
		uint _limit{};
	};
}