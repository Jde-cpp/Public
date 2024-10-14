#pragma once
#include "FilterQL.h"
#include <jde/framework/io/json.h>
#define let const auto

namespace Jde::DB{ struct Column; };
namespace Jde::QL{
	struct ColumnQL final{
		Ω QLType( const DB::Column& db, SRCE )ε->string;

		string JsonName;
		mutable sp<DB::Column> SchemaColumnPtr;
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
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6, Start=7, Stop=8 };
	constexpr array<sv,9> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove", "start", "stop" };
	struct MutationQL final{
		MutationQL( sv j, EMutationQL type, const jobject& args, optional<TableQL> resultPtr ):JsonName{j}, Type{type}, Args(args), ResultPtr{resultPtr}{}
		α TableName()Ι->string; //json name=user returns users
		α Input(SRCE)Ε->jobject;
		//template<class T=uint> auto Id( ELogTags tags=ELogTags::None, SRCE )Ε->T{ return Args.Get<T>( "id", sl ); }
		α InputParam( sv name )Ε->jvalue;
		α ParentPK()Ε->uint;
		α ChildPK()Ε->uint;

		string JsonName;
		EMutationQL Type;
		jobject Args;
		optional<TableQL> ResultPtr;
	private:
		mutable string _tableName;
	};
}
#undef let