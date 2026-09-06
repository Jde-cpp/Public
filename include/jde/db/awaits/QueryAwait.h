#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Sql.h>
#include "InlineAwait.h"

namespace Jde::DB{
	struct IDataSource;
	struct Result{
		uint RowsAffected{ 0 };
		vector<Row> Rows;
	};

	struct ΓDB QueryAwait : InlineAwait<Result>{
		using base=InlineAwait<Result>;
		QueryAwait( up<TAwait<Result>>&& awaitable, SRCE )ι:base{sl},_awaitable{move(awaitable)}{}
		α await_ready()ι->bool override;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task;
		up<TAwait<Result>> _awaitable;
	};

	//Every awaitable over IDataSource::Query ends the same way, and this is that ending written once: await the driver's
	//QueryAwait, hand `self`'s promise what `project` makes of the Result, or hand it the exception - IPromise::SetExp's
	//ladder keeps the driver's dynamic type through the runtime_error& catch.  It returns QueryAwait::Task because that
	//is what it awaits (the pairing rule); `self` is the awaitable the caller is suspended on, so it and the `ds` it owns
	//outlive this coroutine and are referenced, while `sql` and `project` are moved into the frame.  `project` may throw
	//- a Scaler with no row does - and that reaches the promise the same way a driver error does.  `ds` is deduced, not
	//IDataSource&, so this can sit above IDataSource's definition and inside the header templates that use it.
	α RunQuery( auto& self, auto& ds, Sql sql, bool outParams, SL sl, auto project )ι->QueryAwait::Task{
		try{
			self.Resume( project( co_await ds.Query(move(sql), outParams, sl) ) );
		}
		catch( runtime_error& e ){
			self.ResumeExp( move(e) );
		}
	}
}