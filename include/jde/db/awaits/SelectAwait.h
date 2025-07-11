#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/Row.h>
#include "QueryAwait.h"

namespace Jde::DB{
	struct SelectAwait : TAwaitEx<vector<Row>,QueryAwait::Task>{
		using base=TAwaitEx<vector<Row>,QueryAwait::Task>;
		SelectAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α ΓDB Execute()ι->QueryAwait::Task override;
	private:
		sp<IDataSource> _ds;
		Sql _sql;
	};
}