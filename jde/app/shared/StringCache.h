#pragma once
#include <jde/db/usings.h>
//#include <jde/app/shared/proto/App.FromClient.pb.h>
namespace Jde::App::StringCache{
	using App::Proto::FromClient::EFields;

	α Add( EFields field, StringPK id, str value, ELogTags logTags )ι->bool;
	α AddFile( StringPK& id, str path )ι->bool;
	α AddFunction( StringPK& id, str name )ι->bool;
	α AddMessage( StringPK& id, str m )ι->bool;
	α AddThread( StringPK& id, str name )ι->bool;
	α GetMessage( StringPK id )ι->string;
	α GetFile( StringPK id )ι->string;
	α GetFunction( StringPK id )ι->string;
	α Merge( concurrent_flat_map<StringPK,string>&& files, concurrent_flat_map<StringPK,string>&& functions, concurrent_flat_map<StringPK,string>&& messages )ι->void;
}