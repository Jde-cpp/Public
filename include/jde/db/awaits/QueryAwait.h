#pragma once
#include <jde/framework/co/Await.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Sql.h>

namespace Jde::DB{
	struct IDataSource; struct IRow;
	struct Result{
		uint RowsAffected{ 0 };
		vector<Row> Rows;
	};

	struct ΓDB QueryAwait : TAwait<Result>{
		using base=TAwait<Result>;
		QueryAwait( up<TAwait<Result>>&& awaitable, SRCE )ι:base{sl},_awaitable{move(awaitable)}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task;
		up<TAwait<Result>> _awaitable;
	};
}