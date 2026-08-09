#pragma once
#include "../Row.h"
#include <jde/db/awaits/SelectAwait.h>

#define let const auto

namespace Jde::DB{
	template<class T>
	struct IMapAwait : TAwaitEx<T,SelectAwait::Task>{
		using base=TAwaitEx<T,SelectAwait::Task>;
		IMapAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι:
			base{ sl }, _ds{ move(ds) }, _sql{ move(s) }
		{}
		α Execute()ι->SelectAwait::Task override;
	protected:
		β Process( vector<Row>&& rows )ε->void=0;//not noexcept: Row::Get throws on a NULL key/value, and Execute() catches it -> ResumeExp. noexcept here would std::terminate instead.
	private:
		sp<IDataSource> _ds;
		Sql _sql;
	};
	Ŧ IMapAwait<T>::Execute()ι->SelectAwait::Task{
		try{
			Process( co_await SelectAwait{_ds, move(_sql), base::_sl} );
		}
		catch( runtime_error& e ){
			base::ResumeExp( move(e) );
		}
	}

	template<class K,class V>
	struct MapAwait : IMapAwait<flat_map<K,V>>{
		using base=IMapAwait<flat_map<K,V>>;
		MapAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι : base{ move(ds), move(s), sl }{}
	private:
		α Process( vector<Row>&& rows )ε->void override{
			flat_map<K,V> y;
			for( let& row : rows )
				y.emplace( row.template Get<K>(0), row.template Get<V>(1) );
			base::Resume( move(y) );
		}
	};
}