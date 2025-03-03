#include "HttpRequestAwait.h"
#include <jde/web/client/Jwt.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/framework/str.h>
#include <jde/framework/thread/execution.h>
#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/threading/Thread.h"

#define let const auto

namespace Jde::Web::Mock{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}
	α CertificateLogin( const HttpRequest& req )ι->jobject{
		try{
			req.LogRead();
			Web::Jwt jwt{ Json::AsString(req.Body(), "jwt") };
			if( std::abs(time(nullptr)-jwt.Iat)>60*10 )
				THROW( "Invalid iat.  Expected ~'{}', found '{}'.", time(nullptr), jwt.Iat );

			Crypto::Verify( jwt.Modulus, jwt.Exponent, jwt.HeaderBodyEncoded, jwt.Signature );
			req.SessionInfo->UserPK = {42};
			jobject j{ {"expiration", ToIsoString(req.SessionInfo->Expiration)} };
			req.SessionInfo->IsInitialRequest = true;  //expecting sessionId to be set.
			return j;
		}
		catch( IException& ){
			ASSERT(false);
			return {};
		}
	}

	α EchoResult( const flat_map<string,string>& params )ι->jobject{
		jarray echo;
		for( let& [key,value] : params ){
			if( value.empty() )
				echo.emplace_back( key );
			else{
				jobject j;
				j[key] = value;
				echo.emplace_back(j);
			}
		}
		return jobject{ {"params", echo} };
	}

	α HttpRequestAwait::await_ready()ι->bool{
		optional<jobject> result;
		if( _request.IsGet("/echo") )
			result = EchoResult( _request.Params() );
		else if( _request.IsGet("/Authorization") ){
			_request.ResponseHeaders.emplace( "Authorization", Jde::format("{:x}", _request.SessionInfo->SessionId) );
			result = jobject{};
		}
		else if( _request.IsGet("/timeout") ){
			jobject j;
			let expiration = Chrono::ToClock<Clock,steady_clock>( _request.SessionInfo->Expiration );
			j["value"] = ToIsoString( expiration );
			result = j;
		}
		else if( _request.IsPost("/CertificateLogin") ){
			result = CertificateLogin( _request );
		}
		if( result ){
			_result = HttpTaskResult{ move(*result), move(_request) };
		}
		return _result.has_value();
	}
	α HttpRequestAwait::Suspend()ι->void{
		if( _request.Target()=="/delay" ){
			 _thread = std::jthread( [this,h=_h]()mutable->void {
				Threading::SetThreadDscrptn( "DelayHandler" );
				uint seconds = To<uint>( _request["seconds"] );
				Debug( ELogTags::HttpServerWrite, "server sleeping for {}", seconds );
				std::this_thread::sleep_for( std::chrono::seconds{seconds} );
				Promise()->SetValue( {jobject{}, move(_request)} );
				net::post( *Executor(), [h](){ h.resume(); } );
				Debug( ELogTags::HttpServerWrite, "~/delay handler" );
			});
		}
		else if( _request.Target()=="/BadAwaitable" ){
			_thread = std::jthread( [this,h=_h]()mutable->void {
				Threading::SetThreadDscrptn( "BadAwaitable" );
				h.promise().SetExp( RestException{SRCE_CUR, move(_request), "BadAwaitable"} );
				net::post( *Executor(), [h](){ h.resume(); } );
				Debug( ELogTags::HttpServerWrite, "~/BadAwaitable handler" );
			 });
		}
		else
			ResumeExp( RestException<http::status::not_found>(SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target()) );
	}
	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		ASSERT( Promise() || _result );
		base::AwaitResume();
		return Promise() ? move(*Promise()->Value()) : move(*_result);
	}
}