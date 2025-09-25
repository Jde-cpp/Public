#include <jde/app/shared/StringCache.h>
#include <boost/uuid/uuid_io.hpp>

namespace Jde::App{
	concurrent_flat_map<StringMd5,string> _files;
	concurrent_flat_map<StringMd5,string> _functions;
	concurrent_flat_map<StringMd5,string> _messages;
	concurrent_flat_map<StringMd5,string> _threads;

	α StringCache::Merge( concurrent_flat_map<StringMd5,string>&& files, concurrent_flat_map<StringMd5,string>&& functions, concurrent_flat_map<StringMd5,string>&& messages )ι->void{
		_files.merge( files );
		_functions.merge( functions );
		_messages.merge( messages );
	}
	α Emplace( concurrent_flat_map<StringMd5,string>& map, StringMd5& id, str value )ι->bool{
		if( id==uuid{} )
			id = Logging::Entry::GenerateId( value );
		return map.try_emplace( id, value );
	}
	α StringCache::Add( Proto::FromClient::EFields field, StringMd5 id, str value, ELogTags logTags )ι->bool{
		bool save = false;
		if( value.empty() )
			Critical( logTags, "empty string field: {}, id: {}.", underlying(field), boost::uuids::to_string(id) );
		else if( field==Proto::FromClient::EFields::MessageId )
			save = AddMessage( id, value );
		else if( field==Proto::FromClient::EFields::FileId )
			save = AddFile( id, value );
		else if( field==Proto::FromClient::EFields::FunctionId )
			save = AddFunction( id, value );
		else
			Critical( logTags, "unknown field {}.", underlying(field) );
		return save;
	}

	α StringCache::AddFile( StringMd5 id, str path )ι->bool{ return Emplace( _files, id, path ); }
	α StringCache::AddFunction( StringMd5 id, str name )ι->bool{ return Emplace( _functions, id, name ); }
	α StringCache::AddMessage( StringMd5 id, str m )ι->bool{ return Emplace( _messages, id, m ); }
	α StringCache::AddThread( StringMd5& id, str name )ι->bool{ return Emplace( _threads, id, name ); }

	α Get( StringMd5 id, const concurrent_flat_map<StringMd5,string>& map  )ι->string{ string y; map.cvisit(id, [&y](auto kv){ y=kv.second;}); return y; }
	α StringCache::GetMessage( StringMd5 id )ι->string{ return Get( id, _messages ); }
	α StringCache::GetFile( StringMd5 id )ι->string{ return Get( id, _files ); }
	α StringCache::GetFunction( StringMd5 id )ι->string{ return Get( id, _functions ); }
};