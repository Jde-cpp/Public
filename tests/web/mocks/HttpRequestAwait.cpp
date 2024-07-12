#include "HttpRequestAwait.h"
#include "../../../Framework/source/io/AsioContextThread.h"
#define var const auto

namespace Jde::Web::Mock{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α EchoResult( const flat_map<string,string>& params )ι->json{
		json echo = json::array();
		for( var& [key,value] : params ){
			if( value.empty() )
				echo.push_back( key );
			else{
				json j;
				j[key] = value;
				echo.push_back(j);
			}
		}
		json y;
		y["params"] = echo;
		return y;
	}

	α HttpRequestAwait::await_ready()ι->bool{
		optional<json> result;
		if( _request.Target()=="/echo" )
			result = EchoResult( _request.Params() );
		else if( _request.Target()=="/Authorization" ){
			_request.ResponseHeaders.emplace( "Authorization", Jde::format("{:x}", _request.SessionInfo.SessionId) );
			result = json();
		}
		else if( _request.Target()=="/timeout" ){
			json j;
			var expiration = Chrono::ToClock<Clock,steady_clock>( _request.SessionInfo.Expiration );
			j["value"] = ToIsoString( expiration );
			result = j;
		}
		if( result ){
			_result = HttpTaskResult{move(_request), move(*result)};
		}
		return _result.has_value();
	}
	α HttpRequestAwait::await_suspend( base::Handle h )ε->void{
		base::await_suspend(h);
		if( _request.Target()=="/delay" ){
			 _thread = std::jthread( [this,h]()mutable->void{
				Threading::SetThreadDscrptn( "DelayHandler" );
				uint seconds = To<uint>( _request["seconds"] );
				std::this_thread::sleep_for( std::chrono::seconds{seconds} );
				//Promise()->SetRequest( move(_request) );
				Promise()->SetValue( {move(_request),json()} );
				boost::asio::post( *IO::AsioContextThread(), [h](){ h.resume(); } );
				DBGT( HttpServerSentTag(), "~/delay handler" );
			});
		}
		else if( _request.Target()=="/BadAwaitable" ){
			_thread = std::jthread( [this,h]()mutable->void{
				Threading::SetThreadDscrptn( "BadAwaitable" );
				h.promise().SetError( mu<RestException<>>(RestException{SRCE_CUR, move(_request), "BadAwaitable"}) );
				boost::asio::post( *IO::AsioContextThread(), [h](){ h.resume(); } );
				DBGT( HttpServerSentTag(), "~/BadAwaitable handler" );
			 });
		}
		else
			throw RestException<http::status::not_found>( SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() );
	}
	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		ASSERT( Promise() || _result );
		base::AwaitResume();
		return Promise() ? move(*Promise()->Value()) : move(*_result);
	}
}