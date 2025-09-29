#include <jde/framework/io/Cache.h>
#define let const auto
namespace Jde
{
	std::map<string,tuple<double,TP>> _cacheDouble; shared_mutex _cacheDoubleLock;

	std::map<string,sp<void>> Cache2::_cache; shared_mutex Cache2::_cacheLock;

	α Cache2::Has( str id )ι->bool{ sl _{_cacheLock}; return _cache.find( id )!=_cache.end(); }
	α Cache2::Duration( str id )ι->Jde::Duration
	{
		return Settings::FindDuration( Jde::format("/cache/{}/duration",id) ).value_or(
			Settings::FindDuration( "/cache/default/duration" ).value_or( Jde::Duration::max() ));
	}

	α Cache2::Double( str id )ι->double
	{
		sl l{ _cacheDoubleLock };
		auto p = _cacheDouble.find( id );
		return p==_cacheDouble.end() ? std::nan( "" ) : get<0>( p->second );
	}

	//Find out about this...
	α Cache2::SetDouble( str id, double v, TP t )ι->bool
	{
		if( t==TP::max() )
		{
			if( let d{ Duration(id) }; d!=Duration::max() )
				t =  Clock::now()+d;
		}
		auto r = _cacheDouble.emplace( move(id), make_tuple(v,t) );
		if( !r.second )
			r.first->second = make_tuple( v, t );
		return r.second;
	}

	constexpr ELogTags _tags = ELogTags::Cache;
	Cache _instance;
	α Cache::Instance()ι->Cache&{ return _instance; }

	α Cache::InstanceClear( sv name )ι->bool
	{
		unique_lock l{_cacheLock};
		auto p = _cache.find( name );
		let erased = p!=_cache.end();
		if( erased )
		{
			_cache.erase( p );
			TRACE( "Cache::{} erased={}"sv, name, erased );
		}
		return erased;
	}

	α Cache::Double( string id )ι->double
	{
		sl l{ _cacheDoubleLock };
		auto p = _cacheDouble.find( id );
		return p==_cacheDouble.end() ? std::nan( "" ) : get<0>( p->second );
	}
	α Cache::SetDouble( string id, double v, TP expiration )ι->bool
	{
		if( expiration==TP::max() )
		{
			if( let d{ Duration(id) }; d!=Duration::max() )
				expiration =  Clock::now()+d;
		}
		auto r = _cacheDouble.emplace( move(id), make_tuple(v,expiration) );
		if( !r.second )
			r.first->second = make_tuple( v, expiration );
		return r.second;
	}
}