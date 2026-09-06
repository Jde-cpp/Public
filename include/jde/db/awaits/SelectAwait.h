#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/Row.h>
#include "InlineAwait.h"
#include "QueryAwait.h"

namespace Jde::DB{
	struct SelectAwait : InlineAwait<vector<Row>,TAwaitEx<vector<Row>,QueryAwait::Task>>{
		using base=InlineAwait<vector<Row>,TAwaitEx<vector<Row>,QueryAwait::Task>>;
		SelectAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α ΓDB Execute()ι->QueryAwait::Task override;
		//An in-process driver has nothing to await, so the select runs here and the caller carries on without suspending.
		α ΓDB await_ready()ι->bool override;
	private:
		sp<IDataSource> _ds;
		Sql _sql;
	};
}