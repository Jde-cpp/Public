#pragma once
#ifndef CACHE_H
#define CACHE_H
#include <jde/fwk/process/process.h>
#include <jde/fwk/str.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/utils/collections.h>
#include <jde/fwk/log/log.h>

namespace Jde{
#define let const auto
#define Φ Γ Ω
	struct Cache2{
		Φ Has( str id )ι->bool;

		Φ Duration( str id )ι->Jde::Duration;
		Ṫ Emplace( str id )ι->sp<T>;
		Ṫ Get( str id )ι->sp<T>;
		Φ Double( str id )ι->double;
		Φ SetDouble( str id, double v, TP t=TP::max() )ι->bool;
		Ṫ Set( str id, sp<T> p )ι->sp<T>;
		Φ Clear( sv id )ι->bool;
		template<class K,class V> static α GetValue( str n, K id )ι->sp<V>;
	private:
		static std::map<string,sp<void>> _cache; static  shared_mutex _cacheLock;
	};

	Ŧ Cache2::Emplace( str id )ι->sp<T>{
		sp<T> p;
		Jde::Duration d = Cache2::Duration( id );
		Jde::Duration d2 = Jde::Duration::zero();
		if( d <= d2 )
			p = make_shared<T>();
		else{
			ul l{ _cacheLock };
			auto i = _cache.find( id );
			p = i==_cache.end()
				? std::static_pointer_cast<T>( _cache.try_emplace(id, make_shared<T>)->first->second )
				: i->second;
		}
		return p;
	}

	Ŧ Cache2::Get( str id )ι->sp<T>{
		sl l{_cacheLock};
		auto p = _cache.find( id );
		return p==_cache.end() ? sp<T>{} : std::static_pointer_cast<T>( p->second );
	}

	Ŧ Cache2::Set( str id, sp<T> p )ι->sp<T>{
		ul l{_cacheLock};
		if( !p ){
			let erased = _cache.erase( id );
			TRACET( ELogTags::Cache, "Cache::{} erased={}", id, erased );
		}
		else{
			_cache[id] = p;
			TRACET( ELogTags::Cache, "Cache::{} set", id );
		}
		return p;
	}


	struct Cache final{
		Ω Has( str name )ι{ return Instance().InstanceHas( name ); }
		Ω Duration( str /*name*/ )ι{ return Settings::FindDuration( "/cache/default/duration" ).value_or( Duration::max() ); }
		Ṫ Emplace( str name )ι->sp<T>{ return Instance().InstanceEmplace<T>( name ); }
		Ṫ Get( str name )ι->sp<T>{ return Instance().InstanceGet<T>(name); }
		Φ Double( string name )ι->double;
		Φ SetDouble( string name, double v, TP t=TP::max() )ι->bool;
		Ṫ Set( str name, sp<T> p )ι->sp<T>{ return Instance().InstanceSet<T>(name, p); }
		Φ Clear( sv name )ι->bool{ return Instance().InstanceClear( name ); }
		template<class K,class V> static α GetValue( str n, K id )ι->sp<V>{ return Instance().InstanceGetValue<K,V>( n, id ); }

	private:
		α InstanceClear( sv name )ι->bool;
		α InstanceHas( str name )Ι->bool{ sl l{_cacheLock}; return _cache.find( name )!=_cache.end(); }
		Ŧ InstanceGet( str name )ι->sp<T>;
		ẗ InstanceGetValue( str n, K id )ι->sp<V>;
		Ŧ InstanceEmplace( str name )ι->sp<T>;
		Ŧ InstanceSet( str name, sp<T> pValue )ι->sp<T>;
		Φ Instance()ι->Cache&;
		std::map<string,sp<void>,std::less<>> _cache; mutable shared_mutex _cacheLock;
	};

	Ŧ Cache::InstanceGet( str name )ι->sp<T>{
		sl l{_cacheLock};
		auto p = _cache.find( name );
		return p==_cache.end() ? sp<T>{} : std::static_pointer_cast<T>( p->second );
	}

	Ŧ Cache::InstanceEmplace( str name )ι->sp<T>{
		sl l{_cacheLock};
		auto p = _cache.find( name );
		if( p==_cache.end() )
			p = _cache.try_emplace( name, make_shared<T>() ).first;
		return std::static_pointer_cast<T>( p->second );
	}

	ẗ Cache2::GetValue( str cacheName, K id )ι->sp<V>{
		sp<V> pValue;
		sl l{_cacheLock};
		if( auto p = _cache.find( cacheName ); p!=_cache.end() ){
			if( let pMap = std::static_pointer_cast<flat_map<K,sp<V>>>(p->second); pMap ){
				if( auto pItem = pMap->find( id ); pItem != pMap->end() )
					pValue = pItem->second;
			}
		}
		return pValue;
	}

	ẗ Cache::InstanceGetValue( str cacheName, K id )ι->sp<V>{
		sp<V> pValue;
		sl l{_cacheLock};
		if( auto p = _cache.find( cacheName ); p!=_cache.end() ){
			if( let pMap = std::static_pointer_cast<flat_map<K,sp<V>>>(p->second); pMap ){
				if( auto pItem = pMap->find( id ); pItem != pMap->end() )
					pValue = pItem->second;
			}
		}
		return pValue;
	}

	Ŧ Cache::InstanceSet( str name, sp<T> pValue )ι->	sp<T>{
		ul l{_cacheLock};
		if( !pValue ){
			const bool erased = _cache.erase( name );
			TRACET( ELogTags::Cache, "Cache::{} erased={}"sv, name, erased );
		}
		else{
			_cache[name] = pValue;
			TRACET( ELogTags::Cache, "Cache::{} set"sv, name );
		}
		return pValue;
	}
#undef let
#undef Φ
}
#endif