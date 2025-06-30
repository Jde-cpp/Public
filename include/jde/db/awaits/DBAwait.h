#pragma once
#ifndef JDE_DB_AWAIT_H
#define JDE_DB_AWAIT_H

#include "../usings.h"
#include "../exports.h"
#include <jde/db/Row.h>
#include <jde/db/awaits/SelectAwait.h>

#include "../../../../../Framework/source/Cache.h"
#include "../../../../../Framework/source/coroutine/Awaitable.h"
#define let const auto
namespace Jde::DB{
	struct IDataSource;
	using namespace Coroutine;
	using RowΛ=function<void( Row&& )ε>;
	Τ using CoRowΛ=function<void( T& pResult, Row&& r )ε>;

/////////////////////////////////////////////////////////////////////////////////////////////////////
	α ΓDB TAwaitExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(vector<Row>&&)> onRows, function<void(IException&&)> onError, SL sl )ι->SelectAwait::Task;
	Τ class TSelect{  //TODO need a TSelect for scaler, _pResult can be null in that case.
	protected:
		TSelect( sp<IDataSource> ds, Sql&& sql, CoRowΛ<T> fnctn, SL sl )ι:
			_ds{ds}, _fnctn{fnctn}, _sql{move(sql)}, _sl{sl}
		{}
		virtual ~TSelect()=0;
		α Select( TAwait<T>::Handle h )ι->void;
		//α Result()ι->T&{ return _result; }
		α OnRow( Row&& r )ε->void{ _fnctn( _result, move(r) ); }
		T _result{};
		up<IException> _exception;
	private:
		sp<IDataSource> _ds;
		CoRowΛ<T> _fnctn;
		TAwait<T>::Handle _h;
		Sql _sql;
		SL _sl;
	};

	Τ TSelect<T>::~TSelect(){};
	Ŧ TSelect<T>::Select( TAwait<T>::Handle h )ι->void{
		_h = h;
		TAwaitExecute( move(_ds), move(_sql),
			[&](vector<Row>&& rows){
				for( auto&& r : rows )
					OnRow( move(r) );
				_h.resume();
			},
			[&](IException&& e){ _exception = e.Move(); _h.resume(); },
			_sl
		);
	}


	Τ struct TSelectAwait : TAwait<T>, TSelect<T>{
		TSelectAwait( sp<IDataSource> ds, Sql&& sql, CoRowΛ<T> fnctn, SL sl )ι:
			TAwait<T>{sl},TSelect<T>( ds, move(sql), fnctn, sl )
		{}
		α Suspend()ι->void override{ TSelect<T>::Select( TAwait<T>::_h ); }
		α await_resume()ι->T{
			if( TSelect<T>::_exception )
				TSelect<T>::_exception->Throw();
			return TSelect<T>::_result;
		}
	};

	Τ struct CacheAwait final: TSelectAwait<T>{
		CacheAwait( sp<IDataSource> ds, Sql&& sql, CoRowΛ<T> fnctn, string cacheName, SL sl ):
			TSelectAwait<T>{ ds, move(sql), fnctn, sl },
			_cacheName{ move(cacheName) }
		{}
		α await_ready()ι->bool override;
		α await_resume()ι->T override;
	private:
		sp<T> _cache;
		string _cacheName;
	};

	Ŧ CacheAwait<T>::await_ready()ι->bool{
		_cache = Cache::Get<T>( _cacheName );
		return _cache!=nullptr;
	}

	Ŧ CacheAwait<T>::await_resume()ι->T{
		if( _cache )
			return *_cache;
		let y = TSelectAwait<T>::await_resume();
		Cache::Set<T>( _cacheName, ms<T>(y) );
		Trace{ ELogTags::Test, "Cache.sizeof: {}", sizeof(T) };
		return y;
	}
}
#undef let
#endif