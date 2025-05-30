#pragma once
#include "RowAwait.h"
#include "../IRow.h"
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	template<class T>
	struct ScalerAwait : TAwaitEx<T,RowAwait::Task>{
		using base=TAwaitEx<T,RowAwait::Task>;
		ScalerAwait( sp<const IDataSource> ds, Sql&& s, SL sl )ι:ScalerAwait{ move(ds), move(s), false, sl }{}
		ScalerAwait( sp<const IDataSource> ds, Sql&& s, bool storedProc, SL sl )ι: base{ sl }, _ds{ move(ds) }, _sql{ move(s) }, _storedProc{storedProc}{}
		α Execute()ι->RowAwait::Task override;
	private:
		sp<const IDataSource> _ds;
		Sql _sql;
		bool _storedProc;
	};
	Ŧ ScalerAwait<T>::Execute()ι->RowAwait::Task{
		try{
			let rows = co_await RowAwait{ _ds, move(_sql), _storedProc, base::_sl };
			THROW_IF( rows.empty(), "no results." );
			base::ResumeScaler( rows[0]->template Get<T>(0) );
		}
		catch( IException& e ){
			base::ResumeExp( move(e) );
		}
	}
}
