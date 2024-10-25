#pragma once
#ifndef JSON_H
#define JSON_H
//#include <jde/framework/io/json/JValue.h>

namespace Jde{
	α operator<( const jvalue& a, const jvalue& b )ι->bool;
	Ξ operator<=( const jvalue& a, const jvalue& b )ι->bool{ return a<b || a==b; }
	Ξ operator>( const jvalue& a, const jvalue& b )ι->bool{ return b<a; }
	Ξ operator>=( const jvalue& a, const jvalue& b )ι->bool{ return a>b || a==b; }
	//α operator==( const jvalue& a, const jvalue& b )ι->bool{ return a.is_primitive() && !(b<a) && !(b>a); }

	Ŧ Eval( const boost::system::result<T>& x, string&& message, SRCE )ε->T;//TODO forward args...
	namespace Json{
		α ReadJsonNet( fs::path path, SRCE )ε->jobject;
		constexpr sv errorFromat = "'{}' could not convert to {}.";
#define $(type) Eval( v.try_as_##type(), Ƒ(errorFromat, serialize(v), #type), sl )
		α AsValue( const jobject& o, sv path, SRCE )ε->const jvalue&;
		α AsArray( const jvalue& v, SRCE )ε->const jarray&;
		α AsArray( const jobject& o, sv key, SRCE )ε->const jarray&;
		Ξ AsBool( const jvalue& v, SRCE )ε->bool{ return $(bool); }
		Ŧ AsNumber( const jvalue& v, SRCE )ε->T;
		Ŧ AsNumber( const jobject& o, sv path, SRCE )ε->T{ return AsNumber<T>( AsValue(o,path,sl), sl ); }
		Ξ AsSV( const jvalue& v, sv path, SRCE )ε->sv{ return $(string); }
		Ξ AsSV( const jvalue& v, SRCE )ε->sv{ return $(string); }
		α AsSV( const jobject& j, sv key, SRCE )ε->sv;
		Ξ AsString( const jvalue& v, SRCE )ε->string{ return string{ $(string) }; }
		Ξ AsString( const jobject& j, sv key, SRCE )ε->string{ return string{AsSV(j,key,sl)}; }
		α AsObject( const jvalue& v, SRCE )ε->const jobject&;
		α AsObject( const jvalue& v, sv path, SRCE )ε->const jobject&;
		α AsObject( const jobject& j, sv key, SRCE )ε->const jobject&;

#undef $
		Ξ FindValue( const jvalue& j, sv path )ι->optional<jvalue>{ auto y = j.try_at_pointer(path); return y.has_value() ? *y : optional<jvalue>{}; }
		Ξ FindValuePtr( const jvalue& j, sv path )ι->const jvalue*{ auto y = j.try_at_pointer(path); return y.has_value() ? &*y : nullptr; }
		α FindValue( const jobject& j, sv path )ι->const jvalue*;
		α FindArray( const jvalue& j, sv path )ι->const jarray*;
		α FindArray( const jobject& j, sv key )ι->const jarray*;
		Ξ FindBool( const jvalue& j, sv path )ι->optional<bool>{ auto p = FindValue(j,path); return p && p->is_bool() ? p->get_bool() : optional<bool>{}; }
		α FindBool( const jobject& j, sv key )ι->optional<bool>;
		Ξ FindObject( const jvalue& j, sv path )ι->const jobject*{ auto p = FindValuePtr(j,path); return p ? p->if_object() : nullptr; }
		Ξ FindSV( const jvalue& j, sv path )ι->optional<sv>{ auto p = FindValuePtr(j,path); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		Ξ FindString( const jvalue& j, sv path )ι->optional<string>{ auto sv = FindSV(j, path); return sv ? string{ *sv } : optional<string>{}; }
		Ŧ FindNumber( const jvalue& j, sv path )ι->optional<T>;
		Ŧ FindNumberPath( const jobject& j, sv path )ε->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnum( const jobject& j, sv key, ToEnum&& toEnum )ι->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnum( const jvalue& j, sv path, ToEnum&& toEnum )ι->optional<T>;

		α Find( const jvalue& container, const jvalue& item )ι->const jvalue*;

		α FindDefaultArray( const jvalue& j, sv path )ι->const jarray&;
		α FindDefaultArray( const jobject& j, sv key )ι->const jarray&;
		α FindDefaultObject( const jvalue& j, sv path )ι->const jobject&;

		α Kind( boost::json::kind value )ι->string;

		Ŧ FindNumber( const jobject& j, sv key )ι->optional<T>;
		Ξ FindObject( const jobject& j, sv key )ι->const jobject*{ auto p = j.if_contains(key); return p ? p->if_object() : nullptr; }
		Ξ FindSV( const jobject& j, sv key )ι->optional<sv>{ auto p = j.if_contains(key); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		Ξ FindString( const jobject& j, sv key )ι->optional<string>{ return string{ FindSV(j,key).value_or(sv{}) }; }

		Ξ FindDefaultBool( const jobject& j, sv key )ι->bool{ auto p = j.if_contains(key); return p && p->is_bool() ? p->get_bool() : false; }
		Ξ FindDefaultSV( const jobject& j, sv key )ι->sv{ auto p = j.if_contains(key); return p && p->is_string() ? p->get_string() : sv{}; }
		Ξ FindDefaultSV( const jvalue& j, sv path )ι->sv{ auto p = FindSV(j,path); return p ? *p : sv{}; }

		α Parse( sv json, SRCE )ε->jobject;
	}

	Ŧ Json::AsNumber( const jvalue& v, SL sl )ε->T{
		boost::system::result<T> y = v.try_to_number<T>();
		return Eval( y, Ƒ("'{}', Could not convert to number.", serialize(v)), sl );
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
	template<IsEnum T, class ToEnum> α Json::FindEnum( const jobject& j, sv key, ToEnum&& toEnum )ι->optional<T>{
		auto p = FindSV(j, key);
		return p ? toEnum( *p ) : optional<T>{};
	}
	Ŧ Json::FindNumber( const jobject& j, sv member )ι->optional<T>{
		optional<T> y;
		if( auto m = j.if_contains(member); m )
			if( auto v = m->try_to_number<T>(); v )
				y = *v;
		return y;
	}
#define let const auto
	Ŧ Json::FindNumberPath( const jobject& o, sv path )ε->optional<T>{ 
		optional<T> y;
		if( let v = FindValue(o,path); v ){
			if( let n = v->try_to_number<T>(); n )
				y = *n;
		}
		return y; 
	}
}
Ŧ Jde::Eval( const boost::system::result<T>& x, string&& message, SL sl )ε->T{
	if( !x )
		throw CodeException{ x.error(), ELogTags::Parsing, move(message), ELogLevel::Debug, sl };
	return *x;
}
#undef let
#endif