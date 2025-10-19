#pragma once
#include <jde/db/Key.h>
#include "TableQL.h"
#include "../usings.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct TableQL;
	struct MutationQL final{
		MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw, const vector<sp<DB::AppSchema>>& schemas )ε;
		MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw, const sp<DB::AppSchema>& schema )ε;
		Ω IsMutation( sv name )ι->bool;
		Ω ParseCommand( sv name, SRCE )ε->tuple<string,EMutationQL>;
		α TableName()Ι->string; //json name=user returns users
		//α Input(SRCE)Ε->const jobject&;
		template<class T=uint> α Id()Ι->T;
		template<class T=uint> α FindId()Ι->optional<T>;
		α FindKey()ι->optional<DB::Key>;
		α FindParam( sv name )Ι->const jvalue*{ return const_cast<MutationQL*>(this)->FindParam(name); }
		α FindParam( sv name )ι->jvalue*;
		Ŧ Find( sv name )Ι->optional<T>;

		α GetKey(SRCE)ε->DB::Key;
		Ŧ GetPath( sv path, SRCE )Ε->T;
		Ŧ GetRef( sv name, SRCE )ε->T&;
		Ŧ GetRef( sv name, SRCE )Ε->const T&{ return const_cast<MutationQL*>(this)->GetRef<T>(name, sl); }
		α GetParam( sv name, SRCE )ε->jvalue&;
		α GetParam( sv name, SRCE )Ε->const jvalue&{ return const_cast<MutationQL*>(this)->GetParam(name, sl); }
		α ParentPK()Ε->uint;
		α ChildPK()Ε->uint;
		α ToString()Ι->string;

		jobject Args;
		string CommandName;
		sp<DB::Table> DBTable;
		string JsonTableName;
		optional<TableQL> ResultRequest;
		bool ReturnRaw;
		EMutationQL Type;
	};

	Ŧ MutationQL::FindId()Ι->optional<T>{
		return Json::FindNumber<T>( Args, "id" );
	}

	template<> Ξ MutationQL::Find( sv key )Ι->optional<bool>{
		auto p = FindParam( key );
		return p && p->is_bool() ? p->get_bool() : optional<bool>{};
	}

	Ŧ MutationQL::GetPath( sv path, SL sl )Ε->T{ return Json::AsPath<T>( Args, path, sl ); }
	template<> Ξ MutationQL::GetRef( sv key, SL sl )ε->jarray&{ return GetParam( key, sl ).as_array(); }
	template<> Ξ MutationQL::GetRef( sv key, SL sl )ε->jvalue&{ return GetParam( key, sl ); }
	template<> Ξ MutationQL::GetRef( sv key, SL sl )ε->jobject&{ return GetRef<jvalue>( key, sl ).as_object(); }

	Ŧ MutationQL::Id()Ι->T{
		const optional<T> y = FindId<T>();
		ASSERT( y );
		return y.value_or( T{} );
	}
}