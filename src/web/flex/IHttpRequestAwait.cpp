#include <jde/web/flex/IHttpRequestAwait.h>

namespace Jde::Web::Flex{
	HttpTaskResult::HttpTaskResult( HttpTaskResult&& rhs )ι:
		Request{ move(rhs.Request) },
		Json( move(rhs.Json) )
	{}

	α HttpTaskResult::operator=( HttpTaskResult&& rhs )ι->HttpTaskResult&{
		if( rhs.Request )
			Request.emplace( move(*rhs.Request) );
		else
			Request.reset();
		std::cout << "rhs.Json=" << rhs.Json.dump() << std::endl;
		Json=move( rhs.Json );
		std::cout << "Json=" << Json.dump() << std::endl;
		return *this;
	}

	α HttpTask::promise_type::unhandled_exception()ι->void{
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

	IHttpRequestAwait::~IHttpRequestAwait(){}
	α IHttpRequestAwait::await_resume()ε->HttpTaskResult{
		ASSERT( _pPromise );
		if( _pPromise )
			_pPromise->TestException();
		return _pPromise ? _pPromise->MoveResult() : HttpTaskResult{};
	}
}