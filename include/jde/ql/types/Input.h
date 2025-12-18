#pragma once
#include <jde/db/Key.h>

#define let const auto
namespace Jde::QL{
	struct Input{
		Ŧ Find( sv name )Ι->optional<T>;
		template<class T=jvalue> α FindPtr( sv name )Ι->const T*;
		template<class T=jvalue> α As( sv name, SRCE )Ε->const T&;
		Ŧ AsNumber( sv name, SRCE )Ε->T;
		Ŧ TryNumber( sv name )Ι->optional<T>;
		template<class T=uint> α Id()Ι->T;
		template<class T=uint> α FindId()Ι->optional<T>;
		α FindKey()ι->optional<DB::Key>;
		α GetKey(SRCE)ε->DB::Key;
		Ŧ GetPath( sv path, SRCE )Ε->T;
		α ExtrapolateVariables()Ι->jobject;

		jobject Args;
		sp<jobject> Variables;
	};

	template<> Ξ Input::FindPtr<jvalue>( sv name )Ι->const jvalue*{
		auto value = Args.if_contains( name );
		if( value && value->is_string() ){
			constexpr sv escape{ "\b$" };
			if( value->get_string().starts_with(escape) )
				value = Variables->if_contains( sv{value->get_string()}.substr(2) );
		}
		//if( auto param = value && value->is_string() && value->get_string().starts_with("\\b$") ? sv{value->get_string()} : sv{}; param.size() )
		//	value = Variables->if_contains( param.substr(3) );
		return value;
	}
	template<> Ξ Input::FindPtr( sv name )Ι->const jstring*{
		auto value = FindPtr<jvalue>( name );
		return value && value->is_string() ? &value->get_string() : nullptr;
	}
	template<> Ξ Input::FindPtr( sv name )Ι->const jobject*{
		auto value = FindPtr<jvalue>( name );
		return value && value->is_object() ? &value->get_object() : nullptr;
	}
	template<> Ξ Input::FindPtr( sv name )Ι->const jarray*{
		auto value = FindPtr<jvalue>( name );
		return value && value->is_array() ? &value->get_array() : nullptr;
	}

	template<> Ξ Input::Find( sv key )Ι->optional<bool>{
		auto p = FindPtr<jvalue>( key );
		return p && p->is_bool() ? p->get_bool() : optional<bool>{};
	}


	Ŧ Input::As( sv key, SL sl )Ε->const T&{
		auto p = FindPtr<T>( key );
		THROW_IFSL( !p, "Could not find key '{}' in query: {}, variables: {}", key, serialize(Args), serialize(*Variables) );
		return *p;
	}

	Ŧ Input::TryNumber( sv name )Ι->optional<T>{
		optional<T> y;
		if( auto p = FindPtr( name ); p && p->is_number() ){
			if( auto n = p->try_to_number<T>(); n )
				y = *n;
		}
		return y;
	}
	Ŧ Input::AsNumber( sv name, SL sl )Ε->T{
		auto y = TryNumber<T>( name );
		THROW_IFSL( !y, "Could not find key '{}' in mutation query: {}, variables: {}", name, serialize(Args), serialize(*Variables) );
		return *y;
	}
	Ŧ Input::FindId()Ι->optional<T>{
		return Json::FindNumber<T>( Args, "id" );
	}

	Ŧ Input::GetPath( sv path, SL sl )Ε->T{ return Json::AsPath<T>( Args, path, sl ); }

	Ŧ Input::Id()Ι->T{
		const optional<T> y = FindId<T>();
		ASSERT( y );
		return y.value_or( T{} );
	}
	Ξ Input::GetKey(SL sl)ε->DB::Key{
		let y = FindKey();
		THROW_IFSL( !y, "Could not find id or target in mutation  query: {}, variables: {}", serialize(Args), serialize(*Variables) );
		return *y;
	}
	Ξ Input::FindKey()ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = FindId<uint>(); id )
			y = DB::Key{ *id };
		else if( let target = FindPtr<jstring>("target"); target )
			y = DB::Key{ string{*target} };
		return y;
	}
}
#undef let