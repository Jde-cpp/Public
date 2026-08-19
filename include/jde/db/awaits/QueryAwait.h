#pragma once
#include <jde/fwk/co/Await.h>
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
		α await_ready()ι->bool override;
		α await_resume()ε->Result override;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task;
		up<TAwait<Result>> _awaitable;
		Result _result;      //only when the inner completed in await_ready - a coroutine that never suspends has no
		up<Exception> _exception; //handle, so the promise cannot carry either of these.
		bool _inlined{};
	};
}