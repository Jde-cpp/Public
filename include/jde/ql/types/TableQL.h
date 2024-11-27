#pragma once
#include "FilterQL.h"
#include <jde/framework/io/json.h>
#define let const auto

namespace Jde::DB{ struct Column; struct IRow; };
namespace Jde::QL{
	struct ColumnQL final{
		Ω QLType( const DB::Column& db, SRCE )ε->string;

		string JsonName;
		mutable sp<DB::Column> DBColumn;
	};
	struct JsonMembers{ string ParentTable; string ColumnName; };
	struct TableQL final{
		α DBName()Ι->string;
		α FindColumn( sv jsonName )Ι->const ColumnQL*{ auto p = find_if( Columns, [&](let& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		α EraseColumn( sv jsonName )ι->void{ Columns.erase( remove_if( Columns.begin(), Columns.end(), [&](let& c){return c.JsonName==jsonName;}), Columns.end() ); }
		α FindDBColumn( sp<DB::Column> dbColumn )Ι->const ColumnQL*;
		α FindTable( sv jsonTableName )Ι->const TableQL*;
		α Input()Ε->const jobject&{ return Json::AsObject( Args, "input" ); }
		α IsPlural()Ι{ return JsonName.ends_with( "s" ); }
		α Filter()Ε->FilterQL;
		α ToJson( const DB::IRow& row, const vector<sp<DB::Column>>& dbColumns )Ι->jobject;
		α SetResult( jobject& o, const sp<DB::Column> dbColumn, const DB::Value& value )Ι->void;
		string JsonName;
		jobject Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
		mutable vector<QL::JsonMembers> JsonMembers;
	};
}
#undef let