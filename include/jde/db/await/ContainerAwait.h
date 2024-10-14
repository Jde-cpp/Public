#pragma once
#include "RowAwait.h"
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	template<class K,class V>
	struct MultimapAwait : TAwaitEx<flat_multimap<K,V>,RowAwait::Task>{
		using base=TAwaitEx<flat_multimap<K,V>,RowAwait::Task>;
		MultimapAwait( sp<const IDataSource> ds, Statement&& s, SL sl )ι:
			base{ sl }, _ds{ move(ds) }, _statement{ move(s) }, _sl{ sl }
		{}
		α Execute()ι->RowAwait::Task override;
	private:
		sp<const IDataSource> _ds;
		Statement _statement;
		SL _sl;
	};

	ẗ MultimapAwait<K,V>::Execute()ι->RowAwait::Task{
		try{
			let rows = co_await RowAwait{ _ds, move(_statement), _sl };
			flat_multimap<K,V> _result;
//			for( let& row : rows )
//				_result.emplace( row.Get<K>(0), row.Get<V>(1) );
			base::Resume( move(_result) );
		}
		catch( IException& e ){
			base::ResumeExp( move(e) );
		}
	}
}
