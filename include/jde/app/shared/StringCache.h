#pragma once
#include <jde/db/usings.h>
#include <jde/app/shared/usings.h>

#define Φ ΓAS α
namespace Jde::App::StringCache{
	using Log::Proto::EFields;

	Φ Add( EFields field, StringMd5 id, str value, ELogTags logTags )ι->bool;
	Φ AddFile( StringMd5 id, str path )ι->bool;
	Φ AddFunction( StringMd5 id, str name )ι->bool;
	Φ AddMessage( StringMd5 id, str m )ι->bool;
	α AddThread( StringMd5& id, str name )ι->bool;
	Φ GetMessage( StringMd5 id )ι->string;
	Φ GetFile( StringMd5 id )ι->string;
	α GetFunction( StringMd5 id )ι->string;
	Φ Merge( concurrent_flat_map<StringMd5,string>&& files, concurrent_flat_map<StringMd5,string>&& functions, concurrent_flat_map<StringMd5,string>&& messages )ι->void;
}
#undef Φ