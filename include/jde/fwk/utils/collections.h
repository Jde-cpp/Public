﻿#pragma once
#ifndef COLLECTIONS_H
#define COLLECTIONS_H
#include <algorithm>
#include <forward_list>
#include <sstream>
#include <jde/fwk/io/crc.h>

#define let const auto
namespace Jde{
	Ŧ Find( const T& map, typename T::key_type key )ι->typename std::optional<typename T::mapped_type>{
		auto p = map.find( key );
		return p==map.end() ? std::optional<typename T::mapped_type>{} : p->second;
	}
	Ŧ FindKey( const T& collection, const typename T::mapped_type& value )ι->optional<typename T::key_type>{
		auto pEnd = collection.end();
		auto p = boost::container::find_if( collection.begin(), pEnd, [&value](let& x)ι->bool { return x.second==value; } );
		return p==pEnd ? nullopt : optional<typename T::key_type>{ p->first };
	}
	Ŧ FindDefault( const T& collection, typename T::key_type key )ι->typename T::mapped_type{
		auto pItem = collection.find( key );
		return pItem==collection.end() ? typename T::mapped_type{} : pItem->second;
	}
	Ŧ Reserve( uint size )ι->vector<T>{ vector<T> v; v.reserve( size ); return v; }
	ẗ ReserveMap( uint size )ι->flat_map<K,V>{ flat_map<K,V> v; v.reserve( size ); return v; }
	Ŧ ReserveSet( uint size )ι->flat_set<T>{ flat_set<T> v; v.reserve( size ); return v; }
}
#undef let
#endif