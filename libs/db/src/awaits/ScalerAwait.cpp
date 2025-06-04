#include <jde/db/awaits/ScalerAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde{
	α DB::ScalerAwaitExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(variant<optional<Row>,up<IException>>&&)> callback, SL sl )ι->SelectAwait::Task{
		try{
			auto rows = co_await _ds->SelectAsync( move(_sql), sl );
			callback( rows.size() ? move(rows[0]) : optional<Row>{} );
		}
		catch( IException& e ){
			callback( e.Move() );
		}
	}
}
