#pragma once
#ifndef JSON_H
#define JSON_H
#include <expected>
#include <jde/fwk/exceptions/CodeException.h>
#include <jde/db/Key.h>

#define Φ Γ α
namespace Jde{
	Φ operator<( const jvalue& a, const jvalue& b )ι->bool;
	Ξ operator<=( const jvalue& a, const jvalue& b )ι->bool{ return a<b || a==b; }
	Ξ operator>( const jvalue& a, const jvalue& b )ι->bool{ return b<a; }
	Ξ operator>=( const jvalue& a, const jvalue& b )ι->bool{ return a>b || a==b; }
	//α operator==( const jvalue& a, const jvalue& b )ι->bool{ return a.is_primitive() && !(b<a) && !(b>a); }
	Ŧ Eval( const boost::system::result<T>& x, string&& message, SRCE )ε->T;//TODO forward args...
	namespace Json{
		Φ AddOrAssign( jvalue& objOrArray, jvalue&& item, SRCE )ε->void;
		Φ Combine( const jobject& a, const jobject& b )ι->jobject;
		Φ Visit( jvalue&& v, function<void(jobject&& o)> op )ε->void;
		Φ Visit( const jvalue& v, function<void(const jvalue& o)> op )ε->void;
		Φ Visit( const jvalue& v, function<void(sv s)> op )ε->void;
		Φ Visit( jvalue& v, function<void(jobject& o)> op )ε->void;
		Φ AddImportPath( fs::path path )ι->void;
		Φ TryReadJsonNet( fs::path path, const optional<vector<fs::path>>& importPaths=nullopt, SRCE )ι->std::expected<jobject,string>;
		Φ ReadJsonNet( fs::path path, const optional<vector<fs::path>>& importPaths=nullopt, SRCE )ε->jobject;
		constexpr sv errorFromat = "'{}' could not convert to {}.";
#define $(type) Eval( v.try_as_##type(), Ƒ(errorFromat, serialize(v), #type), sl )
		Φ AsValue( const jobject& o, sv path, SRCE )ε->const jvalue&;
		Φ AsArray( jvalue& o, SRCE )ε->jarray&;
		Ξ AsArray( const jvalue& v, SRCE )ε->const jarray&{ return AsArray(const_cast<jvalue&>(v), sl); }
		Φ AsArray( const jobject& o, sv key, SRCE )ε->const jarray&;
		Φ AsArrayPath( const jobject& o, sv path, SRCE )ε->const jarray&;
		Ξ AsBool( const jvalue& v, SRCE )ε->bool{ return $(bool); }
		Ξ AsBool( const jobject& o, sv path, SRCE )ε->bool{ return AsBool( AsValue(o,path,sl), sl ); }
		Φ AsKey( const jobject& o, SRCE )ε->DB::Key;
		Ŧ AsNumber( const jvalue& v, SRCE )ε->T;
		Ŧ AsNumber( const jobject& o, sv path, SRCE )ε->T{ return AsNumber<T>( AsValue(o,path,sl), sl ); }
		Ŧ AsPath( const jobject& o, sv path, SRCE )ε->T;
		Ξ AsSV( const jvalue& v, sv /*path*/, SRCE )ε->sv{ return $(string); }
		Ξ AsSV( const jvalue& v, SRCE )ε->sv{ return $(string); }
		Φ AsSV( const jobject& o, sv key, SRCE )ε->sv;
		Φ AsSVPath( const jobject& o, sv path, SRCE )ε->sv;
		Ξ AsString( const jvalue& v, SRCE )ε->string{ return string{ $(string) }; }
		Ξ AsString( const jobject& o, sv key, SRCE )ε->string{ return string{AsSV(o,key,sl)}; }
		Φ AsObject( jvalue& v, SRCE )ε->jobject&;
		Ξ AsObject( const jvalue& v, SRCE )ε->const jobject&{ return AsObject(const_cast<jvalue&>(v), sl); }
		Φ AsObject( const jvalue& v, sv path, SRCE )ε->const jobject&;
		Φ AsObject( jobject& o, sv key, SRCE )ε->jobject&;
		Ξ AsObject( const jobject& o, sv key, SRCE )ε->const jobject&{ return AsObject(const_cast<jobject&>(o), key, sl); }
		α AsObjectPath( const jobject& o, sv path, SRCE )ε->const jobject&;
		Φ AsTimePoint( const jobject& o, sv key, SRCE )ε->TimePoint;

#undef $
		Ξ FindValue( const jvalue& v, sv path )ι->optional<jvalue>{ auto y = v.try_at_pointer(path); return y.has_value() ? *y : optional<jvalue>{}; }
		Ξ FindValuePtr( const jvalue& v, sv path )ι->const jvalue*{ auto y = v.try_at_pointer(path); return y.has_value() ? &*y : nullptr; }
		Φ FindValue( jobject& o, sv path )ι->jvalue*;
		Ξ FindValue( const jobject& o, sv path )ι->const jvalue*{ jvalue* v = FindValue(const_cast<jobject&>(o), path); return v; }
		α FindArray( const jvalue& v, sv path )ι->const jarray*;
		Φ FindArray( const jobject& o, sv key )ι->const jarray*;
		Ξ FindBool( const jvalue& v, sv path )ι->optional<bool>{ auto p = FindValue(v,path); return p && p->is_bool() ? p->get_bool() : optional<bool>{}; }
		Φ FindBool( const jobject& o, sv key )ι->optional<bool>;
		Ξ FindObject( const jvalue& v, sv path )ι->const jobject*{ auto p = FindValuePtr(v,path); return p ? p->if_object() : nullptr; }
		Ξ FindSV( const jvalue& v, sv path )ι->optional<sv>{ auto p = FindValuePtr(v,path); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		Ξ FindSV( const jobject& o, sv key )ι->optional<sv>{ auto p = o.if_contains(key); return p && p->is_string() ? p->get_string() : optional<sv>{}; }
		α FindSVPath( const jobject& o, sv path )ι->optional<sv>;

		Ξ FindString( const jvalue& v, sv path )ι->optional<string>{ auto sv = FindSV(v, path); return sv ? string{ *sv } : optional<string>{}; }
		Ŧ FindNumber( const jvalue& v, sv path )ι->optional<T>;
		Ŧ FindNumberPath( const jobject& o, sv path )ε->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnum( const jobject& o, sv key, ToEnum&& toEnum )ι->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnumPath( const jobject& o, sv path, ToEnum&& toEnum )ι->optional<T>;
		template<IsEnum T, class ToEnum> α FindEnum( const jvalue& v, sv path, ToEnum&& toEnum )ι->optional<T>;

		Φ Find( const jvalue& container, const jvalue& item )ι->const jvalue*;

		α FindDefaultArray( const jvalue& v, sv path )ι->const jarray&;
		Φ FindDefaultArray( const jobject& o, sv key )ι->const jarray&;
		Φ FindDefaultObject( const jvalue& v, sv path )ι->const jobject&;
		Φ FindDefaultObject( const jobject& o, sv key )ι->const jobject&;
		Φ FindDefaultObjectPath( const jobject& o, sv path )ι->const jobject&;

		Φ Kind( boost::json::kind value )ι->string;

		Ŧ FindKey( const jobject& o, sv key="id" )ι->optional<T>;
		Ŧ FindNumber( const jobject& o, sv key )ι->optional<T>;
		Ξ FindObject( const jobject& o, sv key )ι->const jobject*{ auto p = o.if_contains(key); return p ? p->if_object() : nullptr; }
		Ξ FindObject( jobject& o, sv key )ι->jobject*{ auto p = o.if_contains(key); return p ? p->if_object() : nullptr; }
		Φ FindString( const jobject& o, sv key )ι->optional<string>;
		Φ FindDuration( const jobject& o, sv key, ELogLevel level=ELogLevel::Debug, SRCE )ι->optional<Duration>;
		Φ FindTimePoint( const jobject& o, sv key )ι->optional<TimePoint>;
		Φ FindTimeZone( const jobject& o, sv key, const std::chrono::time_zone& dflt, ELogLevel level=ELogLevel::Debug, SRCE )ι->const std::chrono::time_zone&;

		Ξ FindDefaultBool( const jobject& o, sv key )ι->bool{ auto p = o.if_contains(key); return p && p->is_bool() ? p->get_bool() : false; }
		Ξ FindDefaultSV( const jobject& o, sv key )ι->sv{ auto p = o.if_contains(key); return p && p->is_string() ? p->get_string() : sv{}; }
		Ξ FindDefaultSV( const jvalue& v, sv path )ι->sv{ auto p = FindSV(v,path); return p ? *p : sv{}; }

		Φ Parse( sv json, SRCE )ε->jobject;
		Φ ParseValue( string&& json, SRCE )ε->jvalue;
		Ŧ FromArray( const jarray& a, SRCE )ε->vector<T>;
		Ŧ ToVector( const jvalue& a, SRCE )ε->vector<T>;
	}

	Ŧ Json::AsNumber( const jvalue& v, SL sl )ε->T{
		boost::system::result<T> y = v.try_to_number<T>();
		return Eval( y, Ƒ("'{}', Could not convert to number.", serialize(v)), sl );
	}

	Ŧ Json::AsPath( const jobject& o, sv path, SL sl )ε->T{
		return AsNumber<T>( o, path, sl );
	}

	Ŧ Json::FindNumber( const jvalue& v, sv path )ι->optional<T>{
		auto p = path.size() ? FindValue(v,path) : optional<jvalue>{v};
		optional<T> y;
		if( p ){
			if( auto n = p->try_to_number<T>(); n )
				y = *n;
		}
		return y;
	}

	template<IsEnum T, class ToEnum> α Json::FindEnum( const jvalue& v, sv path, ToEnum&& toEnum )ι->optional<T>{
		auto p = FindSV( v, path );
		return p ? toEnum( *p ) : optional<T>{};
	}
	template<IsEnum T, class ToEnum> α Json::FindEnum( const jobject& o, sv key, ToEnum&& toEnum )ι->optional<T>{
		auto p = FindSV( o, key );
		return p ? toEnum( *p ) : optional<T>{};
	}
	template<IsEnum T, class ToEnum> α Json::FindEnumPath( const jobject& o, sv path, ToEnum&& toEnum )ι->optional<T>{
		auto v = FindValue( o, path );
		return v && v->is_string() ? toEnum( v->get_string() ) : optional<T>{};
	}
	Ŧ Json::FindNumber( const jobject& o, sv member )ι->optional<T>{
		optional<T> y;
		if( auto m = o.if_contains(member); m )
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
	Ŧ Json::FindKey( const jobject& o, sv key )ι->optional<T>{
		optional<T> y;
		if( let m = o.if_contains(key); m ){
			if( let v = m->try_to_number<typename T::Type>(); v )
				y = T{ *v };
		}
		return y;
	}
	template<> Ξ Json::FromArray<string>( const jarray& a, SL sl )ε->vector<string>{
		vector<string> y;
		for( let& item : a )
			y.push_back( Json::AsString(item, sl) );
		return y;
	}

	Ŧ Json::FromArray( const jarray& a, SL sl )ε->vector<T>{
		vector<T> y;
		for( let& item : a )
			y.push_back( Json::AsNumber<T>(item, sl) );
		return y;
	}
	template<> Ξ Json::ToVector( const jvalue& v, SL sl )ε->vector<string>{
		vector<string> y;
		if( v.is_array() )
			y = FromArray<string>( v.as_array(), sl );
		else if( v.is_string() )
			y.push_back( string{v.get_string()} );
		else
			THROW( "'{}' is not an array or string.", Kind(v.kind()) );
		return y;
	}
	Ŧ Json::ToVector( const jvalue& v, SL sl )ε->vector<T>{
		vector<T> y;
		if( v.is_array() )
			y = FromArray<T>( v.as_array(), sl );
		else
			y.push_back( AsNumber<T>(v, sl) );

		return y;
	}

}
Ŧ Jde::Eval( const boost::system::result<T>& x, string&& message, SL sl )ε->T{
	if( !x )
		throw CodeException{ x.error(), ELogTags::Parsing, move(message), ELogLevel::Debug, sl };
	return *x;
}
#undef Φ
#undef let
#endif