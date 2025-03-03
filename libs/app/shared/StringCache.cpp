#include <jde/app/shared/StringCache.h>

namespace Jde::App{
	concurrent_flat_map<StringPK,string> _files;
	concurrent_flat_map<StringPK,string> _functions;
	concurrent_flat_map<StringPK,string> _messages;
	concurrent_flat_map<StringPK,string> _threads;

	α StringCache::Merge( concurrent_flat_map<StringPK,string>&& files, concurrent_flat_map<StringPK,string>&& functions, concurrent_flat_map<StringPK,string>&& messages )ι->void{
		_files.merge( files );
		_functions.merge( functions );
		_messages.merge( messages );
	}
	α Emplace( concurrent_flat_map<StringPK,string>& map, StringPK& id, str value )ι->bool{
		if( !id )
			id = Calc32RunTime( value );
		return map.try_emplace( id, value );
	}
	α StringCache::Add( Proto::FromClient::EFields field, StringPK id, str value, ELogTags logTags )ι->bool{
		bool save = false;
		if( value.empty() )
			Critical( logTags, "empty string field: {}, id: {}.", underlying(field), id );
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

	α StringCache::AddFile( StringPK& id, str path )ι->bool{ return Emplace( _files, id, path ); }
	α StringCache::AddFunction( StringPK& id, str name )ι->bool{ return Emplace( _functions, id, name ); }
	α StringCache::AddMessage( StringPK& id, str m )ι->bool{ return Emplace( _messages, id, m ); }
	α StringCache::AddThread( StringPK& id, str name )ι->bool{ return Emplace( _threads, id, name ); }

	α Get( StringPK id, const concurrent_flat_map<StringPK,string>& map  )ι->string{ string y; map.cvisit(id, [&y](auto kv){ y=kv.second;}); return y; }
	α StringCache::GetMessage( StringPK id )ι->string{ return Get( id, _messages ); }
	α StringCache::GetFile( StringPK id )ι->string{ return Get( id, _files ); }
	α StringCache::GetFunction( StringPK id )ι->string{ return Get( id, _functions ); }
};