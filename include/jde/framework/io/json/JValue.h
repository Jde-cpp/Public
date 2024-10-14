#pragma once
#include "../../str.h"
#include "JObject.h"

namespace Jde{
/*
	struct JValue : jvalue{
		JValue()=default;
		JValue( jvalue v )ι:jvalue{move(v)}{}

		α operator=( const JValue& x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( nullptr_t x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( sv x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( bool x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( _int x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( uint x )ι->JValue&{ jvalue::operator=(x); return *this; }
		α operator=( double x )ι->JValue&{ jvalue::operator=(x); return *this; }

		α operator<( const JValue& x )Ι->bool;
		α operator<=( const JValue& x )Ι->bool{ return *this<x || *this==x; }
		α operator>( const JValue& x )Ι->bool{ return x<*this; }
		α operator>=( const JValue& x )Ι->bool{ return *this>x || *this==x; }
		α operator==( const JValue& x )Ι->bool{ return is_primitive() && !(x<*this) && !(x>*this); }

		α Find( const JValue& x )Ι->const JValue*;
		template<class T=sv> α Get()Ε->T;
		template<class T=sv> α Get()Ε->T&;
		template<class T=sv> α GetOpt()Ι->optional<T>;
		template<class T=JObject> α GetPtr()Ι->const T*;
		template<class T=sv> α GetDefault()Ι->T;
	};
*/
}