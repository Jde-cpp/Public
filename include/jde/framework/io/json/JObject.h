#pragma once
#ifndef JSON_H
#define JSON_H

namespace Jde{
	struct JObject : jobject{
		JObject()=default;
		JObject( jobject v )ι:jobject{move(v)}{}
		JObject( jvalue v )ι;
		template<class T=sv> α Find( sv path )Ι->optional<T>;
		template<class T=sv> α Get( sv path, SRCE )Ε->T;
		template<class T=JObject> α GetPtr( sv path, SRCE )Ε->const T*;

		template<class T=JObject> α FindPtr( sv path )Ι->const T*;
		template<class T=sv> α FindDefault( sv path )Ι->T{ return Find<T>(path).value_or(T{}); }
		template<class T> α FindEnum( sv path )ι->optional<T>{ return (T)Find<uint>(path); }
	};
}
#endif