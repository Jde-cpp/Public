#pragma once
#include "FilterQL.h"
#include <jde/framework/io/json.h>
#include <jde/db/names.h>
#include <jde/db/generators/SelectClause.h>
#define let const auto

namespace Jde::DB{ struct Column; struct Row; struct Key; };
namespace Jde::QL{
	struct ColumnQL final{
		Ω QLType( const DB::Column& db, SRCE )ε->string;

		string JsonName;
		mutable sp<DB::Column> DBColumn;
	};
	struct JsonMembers{ string ParentTable; string ColumnName; };
	struct TableQL final{
		α DBName()Ι->string;

		α AddFilter( const string& column, const jvalue& value )ι->void;
		α DefaultResult()Ι->jvalue{ return IsPlural() ? jvalue{jarray{}} : jvalue{jobject{}}; }
		α EraseColumn( sv jsonName )ι->void{ Columns.erase( remove_if( Columns.begin(), Columns.end(), [&](let& c){return c.JsonName==jsonName;}), Columns.end() ); }
		α Filter()Ε->FilterQL;
		α FindArgKey()Ι->optional<DB::Key>;
		α FindColumn( sv jsonName )Ι->const ColumnQL*{ auto p = find_if( Columns, [&](let& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		α FindDBColumn( sp<DB::Column> dbColumn )Ι->const ColumnQL*;
		α FindTable( sv jsonPluralName )Ι->const TableQL*;
		α FindTable( sv jsonPluralName )ι->TableQL*;
		α FindTablePrefix( sv jsonPluralName )Ι->const TableQL*;
		α GetTable( sv jsonPluralName, SRCE )ε->TableQL&;
		α IsPlural()Ι->bool{ return DB::Names::IsPlural(JsonName); }
		α SetResult( jobject& o, const sp<DB::Column> dbColumn, DB::Value&& value )Ι->void;
		α ToJson( DB::Row& row, const vector<DB::Object>& dbColumns )Ι->jobject;
		α ToString()Ι->string;
		α TrimColumns( const jobject& fullOutput )Ι->jobject;

		string JsonName;
		jobject Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
		mutable vector<QL::JsonMembers> JsonMembers;
		bool ReturnRaw{true};
	};
}
#undef let