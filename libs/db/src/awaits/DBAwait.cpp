#include <jde/db/awaits/DBAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde{
	α DB::TAwaitExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(vector<Row>&&)> onRows, function<void(IException&&)> onError, SL sl )ι->SelectAwait::Task{
		try{
			auto rows = co_await _ds->SelectAsync( move(_sql), sl );
			onRows( move(rows) );
		}
		catch( IException& e ){
			onError( move(e) );
		}
	}
}