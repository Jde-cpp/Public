#pragma once
#include <jde/db/usings.h>

#define Φ ΓAS α
namespace Jde::App::StringCache{
	using App::Proto::FromClient::EFields;

	Φ Add( EFields field, StringPK id, str value, ELogTags logTags )ι->bool;
	Φ AddFile( StringPK& id, str path )ι->bool;
	Φ AddFunction( StringPK& id, str name )ι->bool;
	Φ AddMessage( StringPK& id, str m )ι->bool;
	α AddThread( StringPK& id, str name )ι->bool;
	Φ GetMessage( StringPK id )ι->string;
	Φ GetFile( StringPK id )ι->string;
	α GetFunction( StringPK id )ι->string;
	Φ Merge( concurrent_flat_map<StringPK,string>&& files, concurrent_flat_map<StringPK,string>&& functions, concurrent_flat_map<StringPK,string>&& messages )ι->void;
}
#undef Φ