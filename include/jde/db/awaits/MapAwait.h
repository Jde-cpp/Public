#pragma once
#include "RowAwait.h"
#include "../IRow.h"
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	template<class T>
	struct IMapAwait : TAwaitEx<T,RowAwait::Task>{
		using base=TAwaitEx<T,RowAwait::Task>;
		IMapAwait( sp<const IDataSource> ds, Sql&& s, SL sl )ι:
			base{ sl }, _ds{ move(ds) }, _sql{ move(s) }
		{}
		α Execute()ι->RowAwait::Task override;
	protected:
		β Process( const vector<up<IRow>>& rows )ι->void=0;
	private:
		sp<const IDataSource> _ds;
		Sql _sql;
	};
	Ŧ IMapAwait<T>::Execute()ι->RowAwait::Task{
		try{
			let rows = co_await RowAwait{ _ds, move(_sql), base::_sl };
			Process( rows );
		}
		catch( IException& e ){
			base::ResumeExp( move(e) );
		}
	}

	template<class K,class V>
	struct MapAwait : IMapAwait<flat_map<K,V>>{
		using base=IMapAwait<flat_map<K,V>>;
		MapAwait( sp<const IDataSource> ds, Sql&& s, SL sl )ι : base{ move(ds), move(s), sl }{}
	private:
		α Process( const vector<up<IRow>>& rows )ι->void override{
			flat_map<K,V> y;
			for( let& row : rows )
				y.emplace( row->template Get<K>(0), row->template Get<V>(1) );
			base::Resume( move(y) );
		}
	};

	template<class K,class V>
	struct MultimapAwait : IMapAwait<flat_multimap<K,V>>{
		using base=IMapAwait<flat_multimap<K,V>>;
		MultimapAwait( sp<const IDataSource> ds, Sql&& s, SL sl )ι : base{ move(ds), move(s), sl }{}
	private:
		α Process( const vector<up<IRow>>& rows )ι->void override{
			flat_multimap<K,V> y;
			for( let& row : rows )
				y.emplace( row->template Get<K>(0), row->template Get<V>(1) );
			base::Resume( move(y) );
		}
	};
}
