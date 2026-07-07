#include <jde/db/awaits/OutAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>

namespace Jde{
	α ΓDB DB::OutExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(DB::Value&&)> onResult, function<void(Exception&&)> onError, SL sl )ι->QueryAwait::Task{
		try{
			auto rows = (co_await _ds->Query(move(_sql), true, sl) ).Rows;
			if( !rows.empty() && rows[0].Size() > 0 )
				onResult( move(rows[0][0]) );
			else
				onError( Exception{"No results"} );
		}
		catch( Exception& e ){
			onError( move(e) );
		}
	}
}