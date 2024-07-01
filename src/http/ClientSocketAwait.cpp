#include <jde/http/ClientSocketAwait.h>

namespace Jde::Http{
	/*
	α ClientSocketTask::unhandled_exception()ι->void{
		//TODO Merge
		try{
			BREAK;
			throw;
		}
		catch( IRestException& e ){
			e.SetLevel( ELogLevel::Critical );
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Critical );
		}
		catch( nlohmann::json::exception& e ){
			Exception{ SRCE_CUR, move(e), ELogLevel::Critical, "json exception - {}", e.what() };
		}
		catch( std::exception& e ){
			Exception{ SRCE_CUR, move(e), ELogLevel::Critical, "std::exception - {}", e.what() };
		}
		catch( ... ){
			Exception{ SRCE_CUR, ELogLevel::Critical, "unknown exception" };
		}
	}
	*/
}