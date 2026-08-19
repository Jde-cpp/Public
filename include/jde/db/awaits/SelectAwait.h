#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/Row.h>
#include "QueryAwait.h"

namespace Jde::DB{
	struct SelectAwait : TAwaitEx<vector<Row>,QueryAwait::Task>{
		using base=TAwaitEx<vector<Row>,QueryAwait::Task>;
		SelectAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α ΓDB Execute()ι->QueryAwait::Task override;
		//An in-process driver has nothing to await, so the select runs here and the caller carries on without suspending.
		α ΓDB await_ready()ι->bool override;
		α ΓDB await_resume()ε->vector<Row> override;
	private:
		sp<IDataSource> _ds;
		Sql _sql;
		vector<Row> _rows;        //only when CompletesInline took the await_ready path: no suspend means no handle,
		up<Exception> _exception; //so the promise cannot carry the result or the error.
		bool _inlined{};
	};
}