#pragma once
#include <jde/db/usings.h>
#include <jde/app/usings.h>

namespace Jde::App::StringCache{
	using Log::Proto::EFields;

	α Add( EFields field, StringMd5 id, str value, ELogTags logTags )ι->bool;
	α AddFile( StringMd5 id, str path )ι->bool;
	α AddFunction( StringMd5 id, str name )ι->bool;
	α AddMessage( StringMd5 id, str m )ι->bool;
	α AddThread( StringMd5& id, str name )ι->bool;
	α GetMessage( StringMd5 id )ι->string;
	α GetFile( StringMd5 id )ι->string;
	α GetFunction( StringMd5 id )ι->string;
	α Merge( concurrent_flat_map<StringMd5,string>&& files, concurrent_flat_map<StringMd5,string>&& functions, concurrent_flat_map<StringMd5,string>&& messages )ι->void;
}