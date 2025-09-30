#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/awaits/QueryAwait.h>
#include <jde/db/generators/Statement.h>

namespace Jde::DB{
	struct IDataSource;
	struct ExecuteAwait : TAwaitEx<uint32,QueryAwait::Task>{
		using base=TAwaitEx<uint32,QueryAwait::Task>;
		ExecuteAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α Execute()ι->QueryAwait::Task override;
	private:
 		sp<IDataSource> _ds;
		Sql _sql;
	};
}