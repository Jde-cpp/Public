#pragma once
#include "FilterQL.h"
#include <jde/io/Json.h>
#define var const auto
namespace Jde::DB{
	struct Column;
	struct ColumnQL final{
		string JsonName;
		mutable const Column* SchemaColumnPtr{nullptr};
		Ω QLType( const DB::Column& db, SRCE )ε->string;
	};

	struct Γ TableQL final{
		α DBName()Ι->string;
		α FindColumn( sv jsonName )Ι->const ColumnQL*{ auto p = find_if( Columns, [&](var& c){return c.JsonName==jsonName;}); return p==Columns.end() ? nullptr : &*p; }
		α FindTable( sv jsonTableName )Ι->const TableQL*{ auto p = find_if( Tables, [&](var& t){return t.JsonName==jsonTableName;}); return p==Tables.end() ? nullptr : &*p; }
		α Input()Ε->const json&{ auto p =Args.find( "input" ); THROW_IF( p == Args.end(), "Could not find 'input' arg." ); return *p;}
		α IsPlural()Ι{ return JsonName.ends_with( "s" ); }
		α Filter()Ε->FilterQL;
		string JsonName;
		json Args;
		vector<ColumnQL> Columns;
		vector<TableQL> Tables;
	};
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6, Start=7, Stop=8 };
	constexpr array<sv,9> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove", "start", "stop" };
	struct Γ MutationQL final{
		MutationQL( sv j, EMutationQL type, const json& args, optional<TableQL> resultPtr ):JsonName{j}, Type{type}, Args(args), ResultPtr{resultPtr}{}
		α TableSuffix()Ι->string; //json name=user returns users
		α Input(SRCE)Ε->json;
		template<class T=uint> auto Id( ELogTags tags=ELogTags::None, SRCE )Ε->T{ return Json::Getε<T>( Args, "id", tags | ELogTags::GraphQL, sl ); }
		α InputParam( sv name )Ε->json;
		α ParentPK()Ε->PK;
		α ChildPK()Ε->PK;

		string JsonName;
		EMutationQL Type;
		json Args;
		optional<TableQL> ResultPtr;
	private:
		mutable string _tableSuffix;
	};
}
#undef var