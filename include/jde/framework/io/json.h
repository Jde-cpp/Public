#pragma once
#ifndef JSON_H
#define JSON_H
#include <jde/framework/io/json/JValue.h>

namespace Jde{
	α operator<( const jvalue& a, const jvalue& b )ι->bool;
	Ξ operator<=( const jvalue& a, const jvalue& b )ι->bool{ return a<b || a==b; }
	Ξ operator>( const jvalue& a, const jvalue& b )ι->bool{ return b<a; }
	Ξ operator>=( const jvalue& a, const jvalue& b )ι->bool{ return a>b || a==b; }
	//α operator==( const jvalue& a, const jvalue& b )ι->bool{ return a.is_primitive() && !(b<a) && !(b>a); }

	Ŧ Eval( const boost::system::result<T>& x, string&& message, SRCE )ε->T;//TODO forward args...
	namespace Json{
		constexpr sv errorFromat = "'{}' could not convert to {}.";
#define $(type) Eval( j.try_as_##type(), Ƒ(errorFromat, serialize(j), #type), sl )
		Ξ AsBool( const jvalue& j, SRCE )ε->bool{ return $(bool); }
		Ŧ AsNumber( const jvalue& j, SRCE )ε->T;
		Ξ AsSV( const jvalue& j, SRCE )ε->sv{ return $(string); }
		Ξ AsString( const jvalue& j, SRCE )ε->string{ return string{ $(string) }; }
		α AsObject( const jvalue& j, SRCE )ε->const jobject&;
		α AsObject( const jvalue& j, sv path, SRCE )ε->const jobject&;

#undef $
		Ξ FindValue( const jvalue& j, sv path )ι->optional<jvalue>{ auto y = j.try_at_pointer(path); return y.has_value() ? *y : optional<jvalue>{}; }
		Ξ FindValuePtr( const jvalue& j, sv path )ι->const jvalue*{ auto y = j.try_at_pointer(path); return y.has_value() ? &*y : nullptr; }
		Ξ FindArray( const jvalue& j, sv path )ι->const jarray*{ auto p = FindValuePtr(j,path); return p ? p->if_array() : nullptr; }
		Ξ FindBool( const jvalue& j, sv path )ι->optional<bool>{ auto p = FindValue(j,path); return p && p->is_bool() ? p->get_bool() : optional<bool>{}; }
		Ξ FindObject( const jvalue& j, sv path )ι->const jobject*{ auto p = FindValuePtr(j,path); return p ? p->if_object() : nullptr; }
		Ξ FindSV( const jvalue& j, sv path )ι->optional<sv>{ auto p = FindValue(j,path); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		Ξ FindString( const jvalue& j, sv path )ι->optional<string>{ auto sv = FindSV(j, path); return sv ? string{ *sv } : optional<string>{}; }
		Ŧ FindNumber( const jvalue& j, sv path )ι->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnum( const jvalue& j, sv path, ToEnum&& toEnum )ι->optional<T>;

		α Find( const jvalue& container, const jvalue& item )ι->const jvalue*;

		α FindDefaultArray( const jvalue& j, sv path )ι->const jarray&;
		α FindDefaultObject( const jvalue& j, sv path )ι->const jobject&;

		α Kind( boost::json::kind value )ι->string;

		Ŧ FindNumber( const jobject& j, sv m )ι->optional<T>;
		Ξ FindObject( const jobject& j, sv m )ι->const jobject*{ auto p = j.if_contains(m); return p ? p->if_object() : nullptr; }
		Ξ FindSV( const jobject& j, sv m )ι->optional<sv>{ auto p = j.if_contains(m); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		Ξ FindString( const jobject& j, sv m )ι->optional<string>{ return string{ FindSV(j,m).value_or(sv{}) }; }

		Ξ FindDefaultBool( const jobject& j, sv m )ι->bool{ auto p = j.if_contains(m); return p && p->is_bool() ? p->get_bool() : false; }
		Ξ FindDefaultSV( const jvalue& j, sv path )ι->sv{ auto p = FindSV(j,path); return p ? *p : sv{}; }

		α Parse( sv json )ε->jobject;
	}

	Ŧ Json::AsNumber( const jvalue& j, SL sl )ε->T{
		boost::system::result<T> y = j.try_to_number<T>();
		return Eval( y, Ƒ("'{}', Could not convert to number.", serialize(j)), sl );
	}

	Ŧ Json::FindNumber( const jvalue& j, sv path )ι->optional<T>{
		auto p = FindValue(j,path);
		optional<T> y;
		if( p ){
			if( auto n = j.try_to_number<T>(); n )
				y = *n;
		}
		return y;
	}

	template<IsEnum T, class ToEnum> α Json::FindEnum( const jvalue& j, sv path, ToEnum&& toEnum )ι->optional<T>{
		auto p = FindSV(j, path);
		return p ? toEnum( *p ) : optional<T>{};
	}
	Ŧ Json::FindNumber( const jobject& j, sv member )ι->optional<T>{
		optional<T> y;
		if( auto m = j.if_contains(member); m )
			if( auto v = m->try_to_number<T>(); v )
				y = *v;
		return y;
	}
}
Ŧ Jde::Eval( const boost::system::result<T>& x, string&& message, SL sl )ε->T{
	if( !x )
		throw CodeException{ x.error(), ELogTags::Parsing, move(message), ELogLevel::Debug, sl };
	return *x;
}
#endif