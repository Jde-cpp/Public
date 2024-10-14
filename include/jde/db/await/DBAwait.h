#pragma once
#ifndef JDE_DB_AWAIT_H
#define JDE_DB_AWAIT_H
#include "../usings.h"
#include <jde/db/IRow.h>

#include "../../../../../Framework/source/Cache.h"
#include "../../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{
	struct IDataSource;
	using namespace Coroutine;
	using RowΛ=function<void( IRow& )ε>;
	Τ using CoRowΛ=function<void( T& pResult, const IRow& r )ε>;
	struct ISelect{
		ISelect( sp<IDataSource> ds )ι:_ds{ds}{}
		β Results()ι->void* = 0;
		β OnRow( const IRow& r )ε->void=0;
		β SelectCo( string sql, vector<Value>&& params, SRCE )ι->up<IAwait>;
	protected:
		sp<IDataSource> _ds;
	};
/////////////////////////////////////////////////////////////////////////////////////////////////////
	Τ class TSelect : public ISelect{  //TODO need a TSelect for scaler, _pResult can be null in that case.
	protected:
		TSelect( IAwait& base_, sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<Value> params )ι:ISelect{ds}, _base{base_},_sql{move(sql)}, _fnctn{fnctn}, _params{move(params)}{}
		virtual ~TSelect()=0;
		α Select( HCoroutine h )->Task;
		α Results()ι->void* override{ return _pResult.release(); }
		α OnRow( const IRow& r )ε->void override{ _fnctn( *_pResult, r ); }
		string ToString()ι;
		IAwait& _base;
		string _sql;
		CoRowΛ<T> _fnctn;
		vector<Value> _params;
	private:
		up<T> _pResult{ mu<T>() };
	};

	Τ struct SelectAwait final: IAwait, TSelect<T>{
		SelectAwait( sp<IDataSource> ds, string sql, CoRowΛ<T> fnctn, vector<Value> params, SL sl )ι:IAwait{sl},TSelect<T>( *this, ds, move(sql), fnctn, move(params) ){}
		α Suspend()ι->void override{ TSelect<T>::Select( move(_h) ); }
	};

	Τ TSelect<T>::~TSelect(){};
	Ŧ TSelect<T>::Select( HCoroutine h )ε->Task{//called from await_suspend, noexcept derived
		try{
			auto pAwait = this->SelectCo( move(_sql), move(_params), _base._sl );
			auto result = co_await *pAwait;
			result.CheckError();
			_base.Set<T>( move(_pResult) );
		}
		catch( IException& e ){
			_base.SetException( e.Move() );
		}
		h.resume();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////
	#define Φ Γ auto
	class ICacheAwait : public IAwait{
		using base=IAwait;
	public:
		ICacheAwait( string name, SL sl ):base{sl},_name{move(name)}{ ASSERT(_name.size()); }
		virtual ~ICacheAwait()=0;
		Φ await_ready()ι->bool;
		Φ await_resume()ι->AwaitResult;
	protected:
		sp<void> _pValue;
		string _name;
	};
	inline ICacheAwait::~ICacheAwait(){};
	Τ struct SelectCacheAwait final: ICacheAwait, TSelect<T>{
		SelectCacheAwait( sp<IDataSource> ds, string sql, string cache, CoRowΛ<T> fnctn, vector<Value> params, SL sl ):ICacheAwait{cache,sl},TSelect<T>{ *this, ds, move(sql), fnctn, move(params) }{}
		α Suspend()ι->void override{ TSelect<T>::Select( _h ); }
		α await_resume()ι->AwaitResult override;
	};

	Ŧ SelectCacheAwait<T>::await_resume()ι->AwaitResult{
		const bool haveValue = (bool)_pValue;
		auto y = haveValue ? AwaitResult{ move(_pValue) } : IAwait::await_resume();
		if( !haveValue && y.HasValue() ){
			sp<T> p{ y. template UP<T>().release() };
			Cache::Set<void>( _name, p );
			y.Set( p );
		}
		return y;
	}
}
#undef Φ
#endif