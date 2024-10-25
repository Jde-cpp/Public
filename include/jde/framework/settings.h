#pragma once
#ifndef JDE_SETTINGS_H //gcc precompiled headers
#define JDE_SETTINGS_H

#include "exports.h"
#include <jde/framework/io/json.h>
//#include <jde/framework/process.h>
//#include "../../Framework/source/DateTime.h"

#define let const auto
#define Φ Γ auto
namespace Jde::Settings{
	Φ Value()ι->const jvalue&;
	α FileStem()ι->string; //used as base path for logs.
	Φ Set( sv path, jvalue v, SRCE )ε->jvalue*;
	α Load()ι->void;

	Ξ AsObject( sv path, SRCE )ι->const jobject&{ return Json::AsObject(Value(), path); }

	Ξ FindArray( sv path )ι->const jarray*{ return Json::FindArray(Value(), path); }
	Ξ FindBool( sv path )ι->optional<bool>{ return Json::FindBool(Value(), path); }
	α FindDuration( sv path )ι->optional<Duration>;
	Ξ FindObject( sv path )ι->const jobject*{ return Json::FindObject(Value(), path); }
	Ξ FindSV( sv path )ι->optional<sv>{ return Json::FindSV(Value(), path); }
	α FindString( sv path )ι->optional<string>;
	Ŧ FindNumber( sv path )ι->optional<T>{ return Json::FindNumber<T>(Value(), path); }

	Ξ FindDefaultArray( sv path )ι->const jarray&{ return Json::FindDefaultArray(Value(),path); };
	Ξ FindDefaultObject( sv path )ι->const jobject&{ return Json::FindDefaultObject(Value(), path); };
	template<IsEnum T, class ToEnum> α FindEnum( sv path, ToEnum&& toEnum )ι->optional<T>{ return Json::FindEnum<T,ToEnum>(Value(), path, toEnum); }
}
namespace Jde{
}
/*	template<> α Find<string>( sv path )ι->optional<string>;
	template<class T=jobject> α FindPtr( sv path )ι->const T*{ return Internal::Value().FindPtr<T>( path ); }

	template<class T=jobject> α GetPtr( sv path )ι->const T*{ return Internal::Value().GetPtr<T>( path ); }

	template<class T=sv> α FindDefault( sv path )ι->T{ return Find<T>(path).value_or(T{}); }
	template<class T> α FindEnum( sv path )ι->optional<T>{ return Internal::Value().FindEnum<T>(path); }
*/
	//α Path()ι->fs::path;
//	Φ Env( sv path, SRCE )ι->optional<string>;
//	Φ Envɛ( sv path, SRCE )ε->string;

/*	struct JsonNumber final{
		using Variant=std::variant<double,_int,uint>;
		JsonNumber( json j )ε;
		JsonNumber( double v )ι:Value{v}{};
		JsonNumber( _int v )ι:Value{v}{};
		JsonNumber( uint v )ι:Value{v}{};

		Variant Value;
	};

	struct Γ Container final{
		using Variant=std::variant<nullptr_t,string,JsonNumber>;
		Container( const json& json )ι;
		Container( const fs::path& jsonFile, SRCE )ε;
		α TryMembers( sv path )ι->flat_map<string,Container>;
		α Have( sv path )ι->bool;
		α FindPath( sv path )Ι->optional<json>;
		Ŧ TryArray( sv path, vector<T> dflt )ι->vector<T>;
		Ŧ Map( sv path )ι->flat_map<string,T>;
		Variant operator[]( sv path )ε;

		α ForEach( sv path, function<void(sv, const nlohmann::json&)> f )ι->void;

		α SubContainer( sv entry )Ε->sp<Container>;
		α TrySubContainer( sv entry )Ι->optional<Container>;
		Ŧ Getɛ( sv path, SRCE )Ε->T;
		template<class T=string> auto Get( sv path )Ι->optional<T>;

		α& Json()ι{ /*ASSERT(_pJson); return *_pJson; }
	private:
		up<json> _pJson;
	};
*/

	//

//	#define $ template<> Ξ
//	$ Container::Getɛ<TimePoint>( sv path, const source_location& sl )Ε->TimePoint{ return DateTime{ Getɛ<string>(path, sl) }.GetTimePoint(); }
//	$ Container::Getɛ<fs::path>( sv path, const source_location& )Ε->fs::path{ let p = Get<string>(path); return p.has_value() ? fs::path{*p} : fs::path{}; }

	// Ŧ Container::Getɛ( sv path, const source_location& sl )Ε->T{
	// 	auto p = Get<T>( path ); if( !p ) throw Exception{ sl, ELogLevel::Debug, "'{}' was not found in settings.", path };//mysql precludes using THROW_IF
	// 	return *p;
	// }

	// $ Container::Get<Duration>( sv path )Ι->optional<Duration>{
	// 	let strng = Get<string>( path );
	// 	optional<std::chrono::system_clock::duration> result;
	// 	if( strng.has_value() )
	// 		Try( [strng, &result](){ result = Chrono::ToDuration(*strng);} );
	// 	return result;
	// }
	// $ Container::Get<ELogLevel>( sv path )Ι->optional<ELogLevel>{
	// 	optional<ELogLevel> level;
	// 	if( auto p = FindPath(path); p ){
	// 		if( p->is_string() )
	// 			level = Str::ToEnum<ELogLevel,array<sv,7>>( ELogLevelStrings, p->get<string>() );
	// 		else if( p->is_number() && p->get<uint8>()<ELogLevelStrings.size() )
	// 			level = (ELogLevel)p->get<int8>();
	// 	}
	// 	return level;
	// }

	// Ξ Container::TryMembers( sv path )ι->flat_map<string,Container>{
	// 	flat_map<string,Container> members;
	// 	auto j = FindPath( path );
	// 	if( j && j->is_object() )
	// 	{
	// 		auto obj = j->get<json::object_t>();
	// 		for( auto& [name,j2] : obj )
	// 			members.emplace( name, Container{ j2 } );
	// 	}
	// 	return members;
	// }

	// $ Container::Get<fs::path>( sv path )Ι->optional<fs::path>{
	// 	auto p = Get<string>( path );
	// 	if( let i{p ? p->find("$(") : string::npos}; i!=string::npos )
	// 		p = Env( path );
	// 	return p ? optional<fs::path>(*p) : nullopt;
	// }

	// Ŧ Container::Get( sv path )Ι->optional<T>{
	// 	auto p = FindPath( path );
	// 	try{
	// 		return p && !p->is_null() ? optional<T>{ p->get<T>() } : nullopt;
	// 	}
	// 	catch( const nlohmann::detail::type_error& e ){
	// 		LOG_ONCE( ELogLevel::Debug, LogTag(), "({}) - {}", path, e.what() );
	// 		return nullopt;
	// 	}
	// }

	// $ Container::TryArray<Container>( sv path, vector<Container> dflt )ι->vector<Container>{
	// 	vector<Container> values;
	// 	if( auto p = FindPath(path); p )
	// 	{
	// 		for( auto& i : p->items() )
	// 			values.emplace_back( i.value() );
	// 	}
	// 	else
	// 		values = move( dflt );

	// 	return values;
	// }
	//Φ Set( sv path, path value )ι->void;

	//α Save( const jobject& j, sv what, SRCE )ε->void;

	//Ŧ TryArray( sv path, vector<T> dflt={} )ι{ return Global().TryArray<T>(path, dflt); }

/*	Ŧ Container::TryArray( sv path, vector<T> dflt )ι->vector<T>{
		vector<T> values;
		if( auto p = FindPath(path); p && p->is_array() ){
			for( let& i : *p )
				values.push_back( i.get<T>() );
		}
		else
			values = move( dflt );
		return values;
	}

	Ŧ Container::Map( sv path )ι->flat_map<string,T>{
		auto pItem = _pJson->find( path );
		flat_map<string,T> values;
		if( pItem!=_pJson->end() )
		{
			for( let& [key,value] : pItem->items() )
				values.emplace( key, value );
		}
		return values;
	}

	Ξ Container::ForEach( sv path, function<void(sv, const nlohmann::json&)> f )ι->void{
		if( auto p = FindPath( path ); p && p->is_object() )
		{
			for( auto&& i : p->items() )
				f( i.key(), i.value() );
		}
	}

	Ŧ Getɛ( sv path, SRCE )ε{ return Global().Getɛ<T>( path, sl ); }
*/
	//$ Get<ELogLevel>( sv path )ι->optional<ELogLevel>{ return Global().Get<ELogLevel>( path ); }
/*	$ Find<Duration>( sv path )ι->optional<Duration>{ return Global().Get<Duration>( path ); }
	Ξ TryMembers( sv path )ι->flat_map<string,Container>{ return Global().TryMembers( path ); }
	Ξ ForEach( sv path, function<void(sv, const nlohmann::json& v)> f )ι->void{ return Global().ForEach(path, f); }

	Τ struct Item{
		Item( sv path, T dflt ):
			Value{ Get<T>(path).value_or(dflt) }
		{}
		operator T(){return Value;}
		const T Value;
	};

	Ŧ TryGetSubcontainer( sv container, sv path )ι->optional<T>{
		optional<T> v;
		if( auto pSub=Global().TrySubContainer( container ); pSub )
			v = pSub->Get<T>( path );
		return v;
	}

	$ TryGetSubcontainer<Container>( sv container, sv path )ι->optional<Container>{
		optional<Container> v;
			if( auto pSub=Global().TrySubContainer( container ); pSub )
				v = pSub->TrySubContainer( path );
		return v;
	}
*/
#undef let
#undef $
#undef Φ
#endif