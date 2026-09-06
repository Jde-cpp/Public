#pragma once
#include <jde/db/awaits/QueryAwait.h>
#include <jde/db/IDataSource.h> //C5: Select() is all this needs, and it is a public pure virtual - no derived type required.

namespace Jde::DB::Sqlite{
	//sqlite is in-process - there is no socket to await, so the query completes before the coroutine would suspend, and
	//InlineAwait carries the result (or the exception) past an await_ready that always answers true.
	//Deliberately not posted to Executor() the way MySqlQueryAwait co_spawns: BlockAwait-over-sqlite is a shipped shape
	//(SelectEnumSync, LocalQL::Upsert, the tests' QuerySync), and handing the work to the executor pool would make those
	//deadlock whenever the blocking thread is itself a pool thread - the wedge sqlite is currently immune to.
	struct SqliteQueryAwait final : InlineAwait<Result>{
		SqliteQueryAwait( sp<IDataSource> ds, Sql&& s, bool /*outParams - native procs return out params as rows*/, SL sl )ι:
			InlineAwait<Result>{ sl }, _ds{ ds }, _sql{ move(s) }
		{}
		α await_ready()ι->bool override{
			return Complete( [&]{
				Result y;
				RowΛ f = [&y]( Row&& r ){ y.Rows.push_back( move(r) ); };
				y.RowsAffected = _ds->Select( move(_sql), f, _sl );
				return y;
			});
		}
	private:
		//no Suspend override: await_ready is always true, so IAwait's empty default is unreachable.
		sp<IDataSource> _ds;
		Sql _sql;
	};
}