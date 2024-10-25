#pragma once
//#include <jde/db/Value.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/generators/WhereClause.h>

namespace Jde::DB{
	struct Value;
	struct Statement final{
//		Statement( string&& sql, vector<DB::Value>&& params )ι:Sql{move(sql)}, Parameters{params} {}
		α Empty()ι->bool{ return Select.Columns.empty(); }
		α Move()ι->Sql;
		SelectClause Select;
		FromClause From;
		WhereClause Where;
		//vector<Value> Params;
	};
}