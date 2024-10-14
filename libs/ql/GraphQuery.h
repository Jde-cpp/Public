#pragma once
#include <jde/db/usings.h>
#include <jde/db/Value.h>
#include <jde/framework/io/json.h>

namespace Jde::DB{ struct Syntax; struct IDataSource; }
namespace Jde::QL{
	struct TableQL;
	α Query( const TableQL& table, jobject& jData, UserPK userId, sp<DB::IDataSource> ds )ε->void;
	α SelectStatement( const TableQL& table, bool includeIdColumn, const DB::Syntax& syntax, string* whereString=nullptr )ι->tuple<string,vector<DB::Value>>;
}