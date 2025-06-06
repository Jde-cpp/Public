#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/IRow.h>
#include <jde/db/generators/Sql.h>


namespace Jde::DB{
	struct IDataSource; struct IRow;
	struct Result{
		uint RowsAffected{ 0 };
		vector<Row> Rows;
	};

	struct QueryAwait : TAwait<Result>{
		using base=TAwait<Result>;
		QueryAwait( up<TAwait<Result>>&& awaitable, SRCE )ι:base{sl},_awaitable{move(awaitable)}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task{
			try{
				Resume( co_await *_awaitable );
			}
			catch( IException& e ){
				ResumeExp( move(e) );
			}
		}
		up<TAwait<Result>> _awaitable;
	};
}