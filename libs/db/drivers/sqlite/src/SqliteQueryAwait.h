#pragma once
#include <jde/db/awaits/QueryAwait.h>
#include "SqliteDataSource.h"

namespace Jde::DB::Sqlite{
	//sqlite is in-process - no socket to await, so Suspend() runs the query and resumes immediately.
	//If large scans ever block coroutine threads, post Main() to a worker pool the way MySqlQueryAwait co_spawns.
	struct SqliteQueryAwait final : TAwait<Result>{
		SqliteQueryAwait( sp<SqliteDataSource> ds, Sql&& s, bool /*outParams - native procs return out params as rows*/, SL sl )ι:
			TAwait<Result>{ sl }, _ds{ ds }, _sql{ move(s) }
		{}
	private:
		α Suspend()ι->void override{
			try{
				Result result;
				RowΛ f = [&result]( Row&& r ){ result.Rows.push_back( move(r) ); };
				result.RowsAffected = _ds->Select( move(_sql), f, _sl );
				Resume( move(result) );
			}
			catch( Exception& e ){
				ResumeExp( move(e) );
			}
		}
		sp<SqliteDataSource> _ds;
		Sql _sql;
	};
}
