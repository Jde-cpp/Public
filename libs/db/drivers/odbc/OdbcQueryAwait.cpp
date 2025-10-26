﻿#include "OdbcQueryAwait.h"
#include <jde/fwk/process/thread.h>

namespace Jde::DB::Odbc{
	α OdbcQueryAwait::Suspend()ι->void{
		std::jthread{ [this](std::stop_token /*st*/) mutable {
			try{
				SetThreadDscrptn( "Odbc" ); 
				Result result;
				function<void( Row&& )> f = [&result]( Row&& r )ι{
					result.Rows.push_back( move(r) );
				};
				result.RowsAffected = _ds->Select( move(_sql), f, _outParams, _sl );
				Resume( move(result) );
			}
			catch( exception& e ){
				ResumeExp( move(e) );
			}
		} }.detach();
	}
}