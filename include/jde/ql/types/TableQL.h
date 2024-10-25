#pragma once
#include "FilterQL.h"
#include <jde/framework/io/json.h>
#define let const auto

namespace Jde::DB{ struct Column; };
namespace Jde::QL{
	struct ColumnQL final{
		Ω QLType( const DB::Column& db, SRCE )ε->string;

		string JsonName;
		mutable sp<DB::Column> DBColumn;
	};

	struct TableQL final{
		α DBName()Ι->string;
		α FindColumn( sv jsonName )Ι->const ColumnQL*{ auto p = find_if( Columns, [&](let& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		α FindTable( sv jsonTableName )Ι->const TableQL*{ auto p = find_if( Tables, [&](let& t){return t.JsonName==jsonTableName;}); return p==Tables.end() ? nullptr : &*p; }
		α Input()Ε->const jobject&{ return Json::AsObject( Args, "input" ); }
		α IsPlural()Ι{ return JsonName.ends_with( "s" ); }
		α Filter()Ε->FilterQL;
		string JsonName;
		jobject Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
	};
}
#undef let