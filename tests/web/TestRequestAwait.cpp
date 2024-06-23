#include "TestRequestAwait.h"
#define var const auto

namespace Jde{
	optional<std::jthread> _webThread;
	α Web::StartTestServer()ι->void{
		_webThread = std::jthread{ []{
			Flex::Start(); //blocking
		} };
		while( !Flex::HasStarted() )
			std::this_thread::sleep_for( 100ms );
		Flex::SetRequestHandler( ms<Flex::TestRequestHandler>() );
	}
	α Web::StopTestServer()ι->void{
		Flex::Stop();
		if( _webThread && _webThread->joinable() ){
			_webThread->join();
			_webThread = {};
		}
	}
}
namespace Jde::Web::Flex{
	TestRequestAwait::TestRequestAwait( HttpRequest&& req, SL sl )ι:
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

	α TestRequestAwait::await_ready()ι->bool{
		optional<json> result;
		if( _input->Target()=="/echo" )
			result = EchoResult( _input->Params() );
		else if( _input->Target()=="/Authorization" ){
			_input->ResponseHeaders.emplace( "Authorization", Jde::format("{:x}", _input->SessionInfo.SessionId) );
			result = json();
		}
		else if( _input->Target()=="/timeout" ){
			json j;
			var expiration = Chrono::ToClock<Clock,steady_clock>( _input->SessionInfo.Expiration );
			j["value"] = ToIsoString( expiration );
			result = j;
		}
		if( result ){
			_result = HttpTaskResult{move(*_input), move(*result)};
		}
		return _result.has_value();
	}
	α TestRequestAwait::await_suspend( HttpCo h )ε->void{
		base::await_suspend(h);
		if( _input->Target()=="/delay" ){
			 _thread = std::jthread( [this,h]()mutable->void{
				Threading::SetThreadDscrptn( "DelayHandler" );
				uint seconds = To<uint>( (*_input)["seconds"] );
				std::this_thread::sleep_for( std::chrono::seconds{seconds} );
				_pPromise->SetRequest( move(*_input) );
				_pPromise->SetResult( json() );
				boost::asio::post( *Flex::GetIOContext(), [h](){ h.resume(); } );
				DBGT( Flex::ResponseTag(), "~/delay handler" );
			});
		}
		else if( _input->Target()=="/BadAwaitable" ){
			_thread = std::jthread( [this,h]()mutable->void{
				Threading::SetThreadDscrptn( "BadAwaitable" );
				h.promise().SetException( RestException(SRCE_CUR, move(*_input), "BadAwaitable") );
				//boost::asio::post( *Flex::GetIOContext(), [h](){ h.resume(); } );
				boost::asio::post( *Flex::GetIOContext(), [h](){ h.resume(); } );
				DBGT( Flex::ResponseTag(), "~/BadAwaitable handler" );
			 });
		}
		else
			throw RestException<http::status::not_found>( SRCE_CUR, move(*_input), "Unknown target '{}'", _input->Target() );
	}
	α TestRequestAwait::await_resume()ε->HttpTaskResult{
		ASSERT( _pPromise || _result );
		if( _pPromise )
			_pPromise->TestException();
		return _pPromise ? _pPromise->MoveResult() : move(*_result);
	}
}