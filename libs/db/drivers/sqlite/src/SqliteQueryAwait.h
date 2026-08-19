#pragma once
#include <jde/db/awaits/QueryAwait.h>
#include <jde/db/IDataSource.h> //C5: Select() is all this needs, and it is a public pure virtual - no derived type required.

namespace Jde::DB::Sqlite{
	//sqlite is in-process - there is no socket to await, so the query completes before the coroutine would suspend.
	//Say that with await_ready rather than suspending and resuming inline: an inline Resume() runs the continuation
	//*underneath* the caller's own frame (Await.h's Resume is a plain h.resume(), no symmetric transfer), so N
	//back-to-back sqlite awaits nested N deep and only unwound when the chain finally suspended for real or finished.
	//await_ready==true means the awaiting coroutine never suspends and simply carries on - no nesting at all.
	//Deliberately not posted to Executor() the way MySqlQueryAwait co_spawns: BlockAwait-over-sqlite is a shipped shape
	//(SelectEnumSync, LocalQL::Upsert, the tests' QuerySync), and handing the work to the executor pool would make those
	//deadlock whenever the blocking thread is itself a pool thread - the wedge sqlite is currently immune to.
	//The result is carried here rather than through the promise because a coroutine that never suspends has no handle:
	//_h stays null, so TAwait::await_resume's Promise() would be null too.  Same shape as CacheAwait's hit path.
	struct SqliteQueryAwait final : TAwait<Result>{
		SqliteQueryAwait( sp<IDataSource> ds, Sql&& s, bool /*outParams - native procs return out params as rows*/, SL sl )ι:
			TAwait<Result>{ sl }, _ds{ ds }, _sql{ move(s) }
		{}
		α await_ready()ι->bool override{
			try{
				RowΛ f = [this]( Row&& r ){ _result.Rows.push_back( move(r) ); };
				_result.RowsAffected = _ds->Select( move(_sql), f, _sl );
			}
			catch( Exception& e ){//virtual Move() keeps the driver's SqliteException - await_resume rethrows the dynamic type.
				_exception = e.Move();
			}
			catch( runtime_error& e ){
				_exception = mu<Exception>( move(e) );
			}
			return true;
		}
		α await_resume()ε->Result override{
			if( _exception )
				_exception->Throw();
			return move( _result );
		}
	private:
		//no Suspend override: await_ready is always true, so IAwait's empty default is unreachable.
		sp<IDataSource> _ds;
		Sql _sql;
		Result _result;
		up<Exception> _exception;
	};
}
