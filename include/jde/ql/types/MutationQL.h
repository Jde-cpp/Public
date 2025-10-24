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
		template<class T=jvalue> α FindPtr( sv name, const jobject& variables={} )Ι->const T*;
		//α FindParam( sv name, const jobject& variables={} )ι->jvalue*;
		Ŧ Find( sv name, const jobject& variables )Ι->optional<T>;

		α GetKey(SRCE)ε->DB::Key;
		Ŧ GetPath( sv path, SRCE )Ε->T;
		//Ŧ GetRef( sv name, const jobject& variables, SRCE )ε->T&;
		template<class T=jvalue>
		α Get( sv name, const jobject& variables, SRCE )Ε->const T&;
		Ŧ AsNumber( sv name, const jobject& variables, SRCE )Ε->T;
		Ŧ TryNumber( sv name, const jobject& variables )Ι->optional<T>;
		//α GetParam( sv name, const jobject& variables={}, SRCE )ε->jvalue&;
		//α GetParam( sv name, const jobject& variables={}, SRCE )Ε->const jvalue&;
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

	template<> Ξ MutationQL::FindPtr( sv name, const jobject& variables )Ι->const jvalue*{
		auto value = Args.if_contains( name );
		if( auto param = value && value->is_string() && value->get_string().starts_with("§$") ? sv{value->get_string()} : sv{}; param.size() )
			value = variables.if_contains( param.substr(2) );
		return value;
	}
	template<> Ξ MutationQL::FindPtr( sv name, const jobject& variables )Ι->const jstring*{
		auto value = FindPtr<jvalue>( name, variables );
		return value && value->is_string() ? &value->get_string() : nullptr;
	}
	template<> Ξ MutationQL::FindPtr( sv name, const jobject& variables )Ι->const jobject*{
		auto value = FindPtr<jvalue>( name, variables );
		return value && value->is_object() ? &value->get_object() : nullptr;
	}
	template<> Ξ MutationQL::FindPtr( sv name, const jobject& variables )Ι->const jarray*{
		auto value = FindPtr<jvalue>( name, variables );
		return value && value->is_array() ? &value->get_array() : nullptr;
	}
	template<> Ξ MutationQL::Find( sv key, const jobject& variables )Ι->optional<bool>{
		auto p = FindPtr<jvalue>( key, variables );
		return p && p->is_bool() ? p->get_bool() : optional<bool>{};
	}

	Ŧ MutationQL::GetPath( sv path, SL sl )Ε->T{ return Json::AsPath<T>( Args, path, sl ); }

	Ŧ MutationQL::Get( sv key, const jobject& variables, SL sl )Ε->const T&{
		auto p = FindPtr<T>( key, variables );
		THROW_IFSL( !p, "Could not find key '{}' in mutation '{}'", key, ToString() );
		return *p;
	}
	Ŧ MutationQL::TryNumber( sv name, const jobject& variables )Ι->optional<T>{
		optional<T> y;
		if( auto p = FindPtr( name, variables ); p && p->is_number() ){
			if( auto n = p->try_to_number<T>(); n )
				y = *n;
		}
		return y;
	}
	Ŧ MutationQL::AsNumber( sv name, const jobject& variables, SL sl )Ε->T{
		auto y = TryNumber<T>( name, variables );
		THROW_IFSL( !y, "Could not find key '{}' in mutation '{}'", name, ToString() );
		return *y;
	}

	Ŧ MutationQL::Id()Ι->T{
		const optional<T> y = FindId<T>();
		ASSERT( y );
		return y.value_or( T{} );
	}
}