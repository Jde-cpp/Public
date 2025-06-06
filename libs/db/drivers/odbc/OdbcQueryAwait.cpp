#include "OdbcQueryAwait.h"

namespace Jde::DB::Odbc{
	α OdbcQueryAwait::Suspend()ι->void{
		std::jthread{ [this](std::stop_token /*st*/) mutable {
			try{
				Result result;
				function<void( Row&& )> f = [&result]( Row&& r )ι{
					result.Rows.push_back( move(r) );
				};
				result.RowsAffected = _ds->Select( move(_sql), f, _sl );
				Resume( move(result) );
			}
			catch( IException& e ){
				ResumeExp( move(e) );
			}
		} }.detach();
	}
}