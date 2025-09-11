#pragma once
#ifndef OPC_UACLIENT_EXCEPTION_H
#define OPC_UACLIENT_EXCEPTION_H
#include <jde/web/server/RestException.h>
#include <jde/opc/UAException.h>
#include "../usings.h"

#define UACε(x) if( let sc = x; sc ) throw UAClientException{ sc, _ptr, requestId, ELogLevel::Debug };
namespace Jde::Web::Server{ struct HttpRequest; }
namespace Jde::Opc::Gateway{
	struct UAClientException : UAException{
		UAClientException( StatusCode sc, ELogLevel level=ELogLevel::Debug, SRCE )ι:UAException{ sc, {}, sl, {level} }{}
		UAClientException( StatusCode sc, UA_Client* ua, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι:
			UAException{ sc, Ƒ("[{:x}.{:x}]{}", (uint)ua, requestId, UAException::Message(sc)), sl, {level} }{}
		UAClientException( StatusCode sc, string description, Handle uaHandle, SRCE )ι:
			UAException{ sc, Ƒ("[{:x}]{}", uaHandle, UAException::Message(sc)), sl }{}
		UAClientException( const UAClientException& e )ι:UAException{e}{} //copy for multiple requests.
		UAClientException( UAClientException&& e )ι:UAException{move(e)}{}
		//Ω Message( StatusCode x, UA_Client* ua, RequestId requestId )ι->string{ return format("[{:x}.{}] - {}", (uint)ua, requestId, UA_StatusCode_name(x)); }

		α Move()ι->up<IException> override{ return mu<UAClientException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		α IsBadSession()Ι->bool{ return Code==UA_STATUSCODE_BADSESSIONIDINVALID; }
		[[noreturn]] α ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void;
	};

	Ξ UAClientException::ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void{
		if( e.Code==UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
			throw Web::Server::RestException<http::status::unauthorized>( move(e), move(request), "Bad identity token" );
		}
		else if( e.Code==UA_STATUSCODE_BADCONNECTIONREJECTED )
			throw Web::Server::RestException<http::status::bad_gateway>{ move(e), move(request), "Opc Server not reachable" };
		throw Web::Server::RestException<>( move(e), move(request) );
	}
}
#endif