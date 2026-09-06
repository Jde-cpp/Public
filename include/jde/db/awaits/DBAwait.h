#pragma once

#include "../usings.h"
#include "../exports.h"
#include <jde/fwk/io/Cache.h>
#include <jde/db/Row.h>
#include <jde/db/awaits/SelectAwait.h>
#define let const auto
namespace Jde::DB{
	struct IDataSource;
	using RowΛ=function<void( Row&& )ε>;
	Τ using CoRowΛ=function<void( T& pResult, Row&& r )ε>;

	//A select folded into one T - the shape behind SelectMap/SelectEnum, where every row is a map entry.  `fold` is
	//applied to each row in order and the T is what the caller is owed; Suspend hands that to RunQuery as the
	//projection, so this is the same coroutine every other awaitable over IDataSource::Query runs, and a fold that
	//throws reaches the caller through the promise like a driver error would.
	Τ struct TSelectAwait : TAwait<T>{
		using base=TAwait<T>;
		TSelectAwait( sp<IDataSource> ds, Sql&& sql, CoRowΛ<T> fold, SL sl )ι:
			base{ sl }, _ds{ move(ds) }, _sql{ move(sql) }, _fold{ move(fold) }
		{}
		α Suspend()ι->void override{
			RunQuery( *this, *_ds, move(_sql), false, base::_sl, [&]( Result&& r ){
				T y{};
				for( auto&& row : r.Rows )
					_fold( y, move(row) );
				return y;
			});
		}
	private:
		sp<IDataSource> _ds;
		Sql _sql;
		CoRowΛ<T> _fold;
	};

	Τ struct CacheAwait final: TSelectAwait<T>{
		CacheAwait( sp<IDataSource> ds, Sql&& sql, CoRowΛ<T> fold, string cacheName, optional<steady_clock::duration> duration, SL sl ):
			TSelectAwait<T>{ move(ds), move(sql), move(fold), sl },
			_cacheName{ move(cacheName) },
			_duration{ duration }
		{}
		α await_ready()ι->bool override;
		α await_resume()ε->T override;
	private:
		sp<const T> _cache;
		string _cacheName;
		optional<steady_clock::duration> _duration;
	};

	Ŧ CacheAwait<T>::await_ready()ι->bool{
		_cache = Cache::Get<T>( _cacheName );
		return _cache!=nullptr;
	}

	Ŧ CacheAwait<T>::await_resume()ε->T{
		if( _cache )
			return *_cache;
		auto y = TSelectAwait<T>::await_resume();
		TRACET( ELogTags::Test, "Cache.sizeof: {}", sizeof(T) );
		return *Cache::Set<T>( _cacheName, move(y), _duration );//move into the cache, copy out once - `Set(name, y); return y;` copies twice.
	}
}
#undef let